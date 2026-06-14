/* brotway-run: launch a host GTK4 app over the brotway Broadway backend.
 *
 * Works for both install layouts, auto-detected at runtime:
 *   - private prefix (Arch): fork lives in /usr/lib/gtk4-brotway, used via
 *     LD_LIBRARY_PATH, leaving the system gtk4 untouched.
 *   - system overlay (deb): the fork *is* libgtk-4 and gtk4-broadwayd is on
 *     PATH, so no LD_LIBRARY_PATH is needed.
 * Starts the fork's broadwayd, runs the app, tears the daemon down on exit.
 * Mirrors the repo's `make run-host`. No deps beyond libc - ships in the bare
 * base image, which carries no shell-friendly Python.
 *
 *   brotway-run gtk4-widget-factory
 *   brotway-run --auto --open gnome-calculator
 *   brotway-run --display :7 --port 9000 nicotine --isolated
 */

#define _GNU_SOURCE
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

/* Tear down broadwayd; safe to call twice and from a signal handler. */
static void cleanup(void)
{
  if (bwd_pid > 0)
    {
      kill(bwd_pid, SIGTERM);
      waitpid(bwd_pid, NULL, 0);
      bwd_pid = 0;
    }
}

static void on_signal(int sig)
{
  cleanup();
  _exit(128 + sig);
}

static void usage(FILE *out)
{
  fputs(
    "usage: brotway-run [options] <gtk4-app> [app-args...]\n"
    "\n"
    "Run a GTK4 app over the brotway Broadway backend (browser-rendered UI).\n"
    "The app runs with GDK_BACKEND=broadway; triple-Shift opens the debug menu.\n"
    "\n"
    "options:\n"
    "  --display :N   Broadway display number (default :5; env BROTWAY_DISPLAY)\n"
    "  --port P       WebUI port (default 8080+N; env BROTWAY_PORT)\n"
    "  --auto         pick the first free display/port pair from the default\n"
    "  --open         open the WebUI in a browser ($BROWSER, else xdg-open)\n"
    "  -h, --help     show this help\n",
    out);
}

static int is_dir(const char *path)
{
  struct stat st;
  return stat(path, &st) == 0 && S_ISDIR(st.st_mode);
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

int main(int argc, char **argv)
{
  const char *disp_arg = getenv("BROTWAY_DISPLAY");
  const char *port_env = getenv("BROTWAY_PORT");
  int disp_set = disp_arg != NULL;
  int port = port_env ? atoi(port_env) : 0;
  int port_set = port_env != NULL;
  int opt_auto = 0, opt_open = 0;
  if (!disp_arg)
    disp_arg = ":5";

  static struct option longopts[] = {
    { "help",    no_argument,       0, 'h' },
    { "auto",    no_argument,       0, 'a' },
    { "open",    no_argument,       0, 'o' },
    { "display", required_argument, 0, 'd' },
    { "port",    required_argument, 0, 'p' },
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
        case 'p': port = atoi(optarg); port_set = 1; break;
        default:  usage(stderr); return 2;
        }
    }

  if (optind >= argc)
    {
      usage(stderr);
      return 2;
    }
  char **cmd = &argv[optind];

  /* Resolve the fork's broadwayd and, on Arch, point LD_LIBRARY_PATH at it. */
  char bwd_path[PATH_MAX];
  char *bwd;
  if (is_dir(PREFIX))
    {
      const char *cur = getenv("LD_LIBRARY_PATH");
      char *ldp;
      if (cur && *cur)
        {
          if (asprintf(&ldp, "%s:%s", PREFIX, cur) < 0)
            return 1;
        }
      else
        ldp = strdup(PREFIX);
      setenv("LD_LIBRARY_PATH", ldp, 1);
      free(ldp);

      snprintf(bwd_path, sizeof bwd_path, "%s/gtk4-broadwayd", PREFIX);
      bwd = access(bwd_path, X_OK) == 0 ? bwd_path : NULL;
    }
  else
    bwd = which("gtk4-broadwayd");

  if (!bwd)
    {
      fputs("brotway-run: gtk4-broadwayd not found (is gtk4-brotway installed?)\n", stderr);
      return 1;
    }

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

  /* Pick display/port. Port stays coupled to the display (8080+N) unless pinned. */
  int disp_num = atoi(disp_arg[0] == ':' ? disp_arg + 1 : disp_arg);
  if (opt_auto)
    while (socket_exists(disp_num) || port_busy(8080 + disp_num))
      disp_num++;
  if (!port_set)
    port = 8080 + disp_num;

  if (!opt_auto && disp_set && socket_exists(disp_num))
    fprintf(stderr, "brotway-run: warning: display :%d already has a Broadway socket\n", disp_num);

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
      execl(bwd, "gtk4-broadwayd", "-p", portstr, disp, (char *) NULL);
      perror("brotway-run: exec broadwayd");
      _exit(127);
    }

  atexit(cleanup);
  signal(SIGINT, on_signal);
  signal(SIGTERM, on_signal);

  nanosleep(&(struct timespec){ .tv_sec = 1 }, NULL);
  printf("brotway Broadway WebUI: http://localhost:%d  (triple-Shift = debug menu)\n", port);
  fflush(stdout);

  if (opt_open)
    {
      const char *browser = getenv("BROWSER");
      char url[64];
      snprintf(url, sizeof url, "http://localhost:%d", port);
      if (fork() == 0)
        {
          int devnull = open("/dev/null", O_WRONLY);
          if (devnull >= 0)
            {
              dup2(devnull, STDOUT_FILENO);
              dup2(devnull, STDERR_FILENO);
            }
          execlp(browser ? browser : "xdg-open", browser ? browser : "xdg-open",
                 url, (char *) NULL);
          _exit(127);
        }
    }

  setenv("GDK_BACKEND", "broadway", 1);
  setenv("BROADWAY_DISPLAY", disp, 1);

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
