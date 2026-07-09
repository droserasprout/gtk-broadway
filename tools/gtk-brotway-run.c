/* brotway-run: launch a host GTK4 app over the brotway Broadway backend.
 *
 * The fork installs into a private prefix (/usr/lib/gtk4-brotway) - both the deb
 * and the Arch package - so we point LD_LIBRARY_PATH there to load it without
 * touching the system gtk4.
 * Starts the fork's broadwayd, runs the app, tears the daemon down on exit.
 * With no app, just serves an empty Broadway display until Ctrl+C - handy for
 * poking at the backend or attaching apps by hand. Mirrors the repo's
 * `make run-host`. No deps beyond libc - ships in the bare base image, which
 * carries no shell-friendly Python.
 *
 *   brotway-run gtk4-widget-factory
 *   brotway-run --auto --open gnome-calculator
 *   brotway-run --display :7 --port 9000 nicotine --isolated
 *   brotway-run --open                       # empty display, Ctrl+C to stop
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <limits.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define PREFIX "/usr/lib/gtk4-brotway"

static pid_t bwd_pid = 0;

/* Tear down broadwayd; safe to call twice and from a signal handler.
 * Bounded reap so we never hang if broadwayd ignores SIGTERM. */
static void cleanup(void)
{
  if (bwd_pid <= 0)
    return;

  kill(bwd_pid, SIGTERM);
  for (int i = 0; i < 20; i++)  /* up to ~1s of 50ms polls */
    {
      if (waitpid(bwd_pid, NULL, WNOHANG) != 0)  /* reaped, or already gone */
        {
          bwd_pid = 0;
          return;
        }
      nanosleep(&(struct timespec){ .tv_nsec = 50 * 1000 * 1000 }, NULL);
    }
  kill(bwd_pid, SIGKILL);          /* unignorable - it will die */
  waitpid(bwd_pid, NULL, 0);
  bwd_pid = 0;
}

static void on_signal(int sig)
{
  cleanup();
  _exit(128 + sig);
}

static void usage(FILE *out)
{
  fputs(
    "usage: brotway-run [options] [gtk4-app] [app-args...]\n"
    "\n"
    "Run a GTK4 app over the brotway Broadway backend (browser-rendered UI).\n"
    "The app runs with GDK_BACKEND=broadway; triple-Shift opens the debug menu.\n"
    "With no app, just serves an empty Broadway display until Ctrl+C.\n"
    "\n"
    "options:\n"
    "  --display :N   Broadway display number (default :5; env BROTWAY_DISPLAY)\n"
    "  --port P       WebUI port (default 8080+N; env BROTWAY_PORT)\n"
    "  --address A    broadwayd bind address, e.g. 0.0.0.0 (env BROTWAY_ADDRESS)\n"
    "  --auto         pick the first free display/port pair from the default\n"
    "  --open         open the WebUI in a browser ($BROWSER, else xdg-open)\n"
    "  -h, --help     show this help\n",
    out);
}

/* Resolve a program to an executable path (PATH search), or NULL. */
static char *which(const char *prog)
{
  if (strchr(prog, '/'))
    return access(prog, X_OK) == 0 ? strdup(prog) : NULL;

  const char *path = getenv("PATH");
  if (!path || !*path)
    path = "/usr/bin:/bin";

  char *dup = strdup(path), *save = NULL, *found = NULL;
  for (char *dir = strtok_r(dup, ":", &save); dir; dir = strtok_r(NULL, ":", &save))
    {
      char buf[PATH_MAX];
      snprintf(buf, sizeof buf, "%s/%s", dir, prog);
      if (access(buf, X_OK) == 0)
        {
          found = strdup(buf);
          break;
        }
    }
  free(dup);
  return found;
}

/* True if a Broadway socket already exists for this display number. */
static int socket_exists(int disp_num)
{
  char path[PATH_MAX];
  const char *rt = getenv("XDG_RUNTIME_DIR");
  if (rt && *rt)
    snprintf(path, sizeof path, "%s/broadway%d.socket", rt, disp_num + 1);
  else
    snprintf(path, sizeof path, "/run/user/%u/broadway%d.socket",
             (unsigned) getuid(), disp_num + 1);

  struct stat st;
  return stat(path, &st) == 0;
}

/* True if a TCP listener already holds this port (probe via bind). */
static int port_busy(int port)
{
  int fd = socket(AF_INET, SOCK_STREAM, 0);
  if (fd < 0)
    return 0;

  struct sockaddr_in addr;
  memset(&addr, 0, sizeof addr);
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  addr.sin_port = htons((unsigned short) port);

  int busy = bind(fd, (struct sockaddr *) &addr, sizeof addr) < 0 && errno == EADDRINUSE;
  close(fd);
  return busy;
}

/* Parse a base-10 int in [lo, hi]; exit(2) on junk or out-of-range. */
static int parse_num(const char *s, const char *what, int lo, int hi)
{
  char *end;
  errno = 0;
  long v = strtol(s, &end, 10);
  if (errno != 0 || end == s || *end != '\0' || v < lo || v > hi)
    {
      fprintf(stderr, "brotway-run: invalid %s '%s' (want %d-%d)\n", what, s, lo, hi);
      exit(2);
    }
  return (int) v;
}

int main(int argc, char **argv)
{
  const char *disp_arg = getenv("BROTWAY_DISPLAY");
  const char *port_env = getenv("BROTWAY_PORT");
  int disp_set = disp_arg != NULL;
  int port = port_env ? parse_num(port_env, "port", 1, 65535) : 0;
  int port_set = port_env != NULL;
  int opt_auto = 0, opt_open = 0;
  const char *address = getenv("BROTWAY_ADDRESS");
  if (!disp_arg)
    disp_arg = ":5";

  static struct option longopts[] = {
    { "help",    no_argument,       0, 'h' },
    { "auto",    no_argument,       0, 'a' },
    { "open",    no_argument,       0, 'o' },
    { "display", required_argument, 0, 'd' },
    { "port",    required_argument, 0, 'p' },
    { "address", required_argument, 0, 'A' },
    { 0, 0, 0, 0 },
  };

  int c;
  /* leading '+' stops at the first non-option, so app args pass through */
  while ((c = getopt_long(argc, argv, "+h", longopts, NULL)) != -1)
    {
      switch (c)
        {
        case 'h': usage(stdout); return 0;
        case 'a': opt_auto = 1; break;
        case 'o': opt_open = 1; break;
        case 'd': disp_arg = optarg; disp_set = 1; break;
        case 'p': port = parse_num(optarg, "port", 1, 65535); port_set = 1; break;
        case 'A': address = optarg; break;
        default:  usage(stderr); return 2;
        }
    }

  /* No app is allowed: serve an empty Broadway display until Ctrl+C. */
  int no_app = optind >= argc;
  char **cmd = no_app ? NULL : &argv[optind];

  /* Point LD_LIBRARY_PATH at the fork prefix so the app and broadwayd load the
   * fork lib, leaving the system gtk4 untouched. */
  const char *ldcur = getenv("LD_LIBRARY_PATH");
  char *ldp;
  if (ldcur && *ldcur)
    {
      if (asprintf(&ldp, "%s:%s", PREFIX, ldcur) < 0)
        return 1;
    }
  else
    ldp = strdup(PREFIX);
  setenv("LD_LIBRARY_PATH", ldp, 1);
  free(ldp);

  char bwd_path[PATH_MAX];
  snprintf(bwd_path, sizeof bwd_path, "%s/gtk4-broadwayd", PREFIX);
  if (access(bwd_path, X_OK) != 0)
    {
      fputs("brotway-run: gtk4-broadwayd not found (is gtk4-brotway installed?)\n", stderr);
      return 1;
    }
  char *bwd = bwd_path;

  if (!no_app)
    {
      /* Verify the app exists up front for a clean error. */
      char *app = which(cmd[0]);
      if (!app)
        {
          fprintf(stderr, "brotway-run: '%s' not found on PATH\n", cmd[0]);
          return 127;
        }
      free(app);

      const char *gdk = getenv("GDK_BACKEND");
      if (gdk && strcmp(gdk, "broadway") != 0)
        fprintf(stderr, "brotway-run: warning: overriding GDK_BACKEND=%s with broadway\n", gdk);

      /* Broadway clients are remote browsers - the client does IME/composition
       * before events reach us. Server-side ibus is the wrong layer (and wants X11
       * symbols this broadway-only lib lacks). Override if needed. */
      if (!getenv("GTK_IM_MODULE"))
        setenv("GTK_IM_MODULE", "simple", 1);
    }

  /* Pick display/port. Port stays coupled to the display (8080+N) unless pinned. */
  int disp_num = parse_num(disp_arg[0] == ':' ? disp_arg + 1 : disp_arg, "display", 0, 99);
  /* --auto steps the display until free; a pinned port stays put (only its
   * socket is stepped, else a busy pinned port would loop forever). */
  if (opt_auto)
    while (socket_exists(disp_num) || (!port_set && port_busy(8080 + disp_num)))
      disp_num++;
  if (!port_set)
    port = 8080 + disp_num;

  if (!opt_auto && disp_set && socket_exists(disp_num))
    fprintf(stderr, "brotway-run: warning: display :%d already has a Broadway socket\n", disp_num);
  if (port_busy(port))
    fprintf(stderr, "brotway-run: warning: port %d already in use\n", port);

  char disp[32], portstr[16];
  snprintf(disp, sizeof disp, ":%d", disp_num);
  snprintf(portstr, sizeof portstr, "%d", port);

  bwd_pid = fork();
  if (bwd_pid < 0)
    {
      perror("brotway-run: fork");
      return 1;
    }
  if (bwd_pid == 0)
    {
      char *bargv[8];
      int n = 0;
      bargv[n++] = (char *) "gtk4-broadwayd";
      bargv[n++] = (char *) "-p";
      bargv[n++] = portstr;
      if (address)
        {
          bargv[n++] = (char *) "-a";
          bargv[n++] = (char *) address;
        }
      bargv[n++] = disp;
      bargv[n] = NULL;
      execv(bwd, bargv);
      perror("brotway-run: exec broadwayd");
      _exit(127);
    }

  atexit(cleanup);
  signal(SIGINT, on_signal);
  signal(SIGTERM, on_signal);

  /* Wait for broadwayd to be ready (its display socket appears), bailing if it
   * died early (port busy, exec failure). Polls up to ~5s, returns as soon as
   * ready - better than a blind sleep on slow hosts / arm64 emulation. */
  int ready = 0;
  for (int i = 0; i < 250; i++)
    {
      if (waitpid(bwd_pid, NULL, WNOHANG) == bwd_pid)
        {
          bwd_pid = 0;
          fputs("brotway-run: broadwayd exited before it was ready\n", stderr);
          return 1;
        }
      if (socket_exists(disp_num))
        {
          ready = 1;
          break;
        }
      nanosleep(&(struct timespec){ .tv_nsec = 20 * 1000 * 1000 }, NULL);
    }
  if (!ready)
    fputs("brotway-run: warning: broadwayd not ready after 5s; continuing\n", stderr);

  printf("brotway Broadway WebUI: http://localhost:%d  (triple-Shift = debug menu)\n", port);
  fflush(stdout);

  if (opt_open)
    {
      const char *browser = getenv("BROWSER");
      char url[64];
      snprintf(url, sizeof url, "http://localhost:%d", port);
      /* Double-fork so the opener reparents to init and never lingers as a
       * zombie - we only ever wait on broadwayd and the app. */
      pid_t opener = fork();
      if (opener == 0)
        {
          if (fork() == 0)
            {
              int devnull = open("/dev/null", O_WRONLY);
              if (devnull >= 0)
                {
                  dup2(devnull, STDOUT_FILENO);
                  dup2(devnull, STDERR_FILENO);
                }
              if (browser)
                {
                  /* $BROWSER may carry args, so run it through the shell to
                   * word-split. url is our own localhost string (no shell
                   * metacharacters), so quoting it is safe. */
                  char cmdbuf[160];
                  snprintf(cmdbuf, sizeof cmdbuf, "%s '%s'", browser, url);
                  execl("/bin/sh", "sh", "-c", cmdbuf, (char *) NULL);
                }
              else
                execlp("xdg-open", "xdg-open", url, (char *) NULL);
              _exit(127);
            }
          _exit(0);  /* intermediate exits immediately; grandchild -> init */
        }
      if (opener > 0)
        waitpid(opener, NULL, 0);  /* reap the intermediate (instant) */
    }

  if (no_app)
    {
      printf("brotway-run: no app given - serving an empty Broadway display. Ctrl+C to stop.\n");
      fflush(stdout);
      for (;;)
        pause();  /* SIGINT/SIGTERM -> on_signal -> cleanup -> _exit */
    }

  setenv("GDK_BACKEND", "broadway", 1);
  setenv("BROTWAY_DISPLAY", disp, 1);

  pid_t app_pid = fork();
  if (app_pid < 0)
    {
      perror("brotway-run: fork");
      return 1;
    }
  if (app_pid == 0)
    {
      execvp(cmd[0], cmd);
      perror(cmd[0]);
      _exit(127);
    }

  int status;
  waitpid(app_pid, &status, 0);
  cleanup();
  return WIFEXITED(status) ? WEXITSTATUS(status) : 128 + WTERMSIG(status);
}
