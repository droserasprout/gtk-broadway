// Protocol stuff

const BROADWAY_NODE_TEXTURE = 0;
const BROADWAY_NODE_CONTAINER = 1;
const BROADWAY_NODE_COLOR = 2;
const BROADWAY_NODE_BORDER = 3;
const BROADWAY_NODE_OUTSET_SHADOW = 4;
const BROADWAY_NODE_INSET_SHADOW = 5;
const BROADWAY_NODE_ROUNDED_CLIP = 6;
const BROADWAY_NODE_LINEAR_GRADIENT = 7;
const BROADWAY_NODE_SHADOW = 8;
const BROADWAY_NODE_OPACITY = 9;
const BROADWAY_NODE_CLIP = 10;
const BROADWAY_NODE_TRANSFORM = 11;
const BROADWAY_NODE_DEBUG = 12;
const BROADWAY_NODE_REUSE = 13;

const BROADWAY_NODE_OP_INSERT_NODE = 0;
const BROADWAY_NODE_OP_REMOVE_NODE = 1;
const BROADWAY_NODE_OP_MOVE_AFTER_CHILD = 2;
const BROADWAY_NODE_OP_PATCH_TEXTURE = 3;
const BROADWAY_NODE_OP_PATCH_TRANSFORM = 4;

const BROADWAY_OP_GRAB_POINTER = 0;
const BROADWAY_OP_UNGRAB_POINTER = 1;
const BROADWAY_OP_NEW_SURFACE = 2;
const BROADWAY_OP_SHOW_SURFACE = 3;
const BROADWAY_OP_HIDE_SURFACE = 4;
const BROADWAY_OP_RAISE_SURFACE = 5;
const BROADWAY_OP_LOWER_SURFACE = 6;
const BROADWAY_OP_DESTROY_SURFACE = 7;
const BROADWAY_OP_MOVE_RESIZE = 8;
const BROADWAY_OP_SET_TRANSIENT_FOR = 9;
const BROADWAY_OP_DISCONNECTED = 10;
const BROADWAY_OP_SURFACE_UPDATE = 11;
const BROADWAY_OP_SET_SHOW_KEYBOARD = 12;
const BROADWAY_OP_UPLOAD_TEXTURE = 13;
const BROADWAY_OP_RELEASE_TEXTURE = 14;
const BROADWAY_OP_SET_NODES = 15;
const BROADWAY_OP_ROUNDTRIP = 16;
const BROADWAY_OP_SET_CLIPBOARD = 17;
const BROADWAY_OP_REQUEST_CLIPBOARD = 18;
const BROADWAY_OP_SET_INPUT_REGION = 19;
const BROADWAY_OP_REASSERT_POINTER = 20;
const BROADWAY_OP_OPEN_URI = 21;
const BROADWAY_OP_SESSION = 22;
const BROADWAY_OP_PONG = 23;
const BROADWAY_OP_DEBUG_FLASH = 24;

/* Latin 'v'/'V' keysyms, used to recognise the paste shortcut (Ctrl+V and
 * Ctrl+Shift+V) so the browser's native 'paste' event is allowed to fire. */
const KEYSYM_v = 118;
const KEYSYM_V = 86;

const BROADWAY_EVENT_ENTER = 0;
const BROADWAY_EVENT_LEAVE = 1;
const BROADWAY_EVENT_POINTER_MOVE = 2;
const BROADWAY_EVENT_BUTTON_PRESS = 3;
const BROADWAY_EVENT_BUTTON_RELEASE = 4;
const BROADWAY_EVENT_TOUCH = 5;
const BROADWAY_EVENT_SCROLL = 6;
const BROADWAY_EVENT_KEY_PRESS = 7;
const BROADWAY_EVENT_KEY_RELEASE = 8;
const BROADWAY_EVENT_GRAB_NOTIFY = 9;
const BROADWAY_EVENT_UNGRAB_NOTIFY = 10;
const BROADWAY_EVENT_CONFIGURE_NOTIFY = 11;
const BROADWAY_EVENT_SCREEN_SIZE_CHANGED = 12;
const BROADWAY_EVENT_FOCUS = 13;
const BROADWAY_EVENT_ROUNDTRIP_NOTIFY = 14;
const BROADWAY_EVENT_CLIPBOARD_CONTENTS = 15;
const BROADWAY_EVENT_PING = 16;
/* Browser->daemon "summon the debug menu". The daemon intercepts it (like
 * CLIPBOARD_CONTENTS) and never forwards it to GTK clients. */
const BROADWAY_EVENT_MENU = 17;

const DISPLAY_OP_REPLACE_CHILD = 0;
const DISPLAY_OP_APPEND_CHILD = 1;
const DISPLAY_OP_INSERT_AFTER_CHILD = 2;
const DISPLAY_OP_APPEND_ROOT = 3;
const DISPLAY_OP_SHOW_SURFACE = 4;
const DISPLAY_OP_HIDE_SURFACE = 5;
const DISPLAY_OP_DELETE_NODE = 6;
const DISPLAY_OP_MOVE_NODE = 7;
const DISPLAY_OP_RESIZE_NODE = 8;
const DISPLAY_OP_RESTACK_SURFACES = 9;
const DISPLAY_OP_DELETE_SURFACE = 10;
const DISPLAY_OP_CHANGE_TEXTURE = 11;
const DISPLAY_OP_CHANGE_TRANSFORM = 12;

// GdkCrossingMode
const GDK_CROSSING_NORMAL = 0;
const GDK_CROSSING_GRAB = 1;
const GDK_CROSSING_UNGRAB = 2;

// GdkModifierType
const GDK_SHIFT_MASK    = 1 << 0;
const GDK_LOCK_MASK     = 1 << 1;
const GDK_CONTROL_MASK  = 1 << 2;
const GDK_ALT_MASK      = 1 << 3;
const GDK_BUTTON1_MASK  = 1 << 8;
const GDK_BUTTON2_MASK  = 1 << 9;
const GDK_BUTTON3_MASK  = 1 << 10;
const GDK_BUTTON4_MASK  = 1 << 11;
const GDK_BUTTON5_MASK  = 1 << 12;
const GDK_SUPER_MASK    = 1 << 26;
const GDK_HYPER_MASK    = 1 << 27;
const GDK_META_MASK     = 1 << 28;


var useDataUrls = window.location.search.includes("datauri");

/* check if we are on Android and using Chrome */
var isAndroidChrome = false;
{
    var ua = navigator.userAgent.toLowerCase();
    if (ua.indexOf("android") > -1 && ua.indexOf("chrom") > -1) {
	isAndroidChrome = true;
    }
}
/* check for the passive option for Event listener */
let passiveSupported = false;
try {
    const options = {
	get passive() { // This function will be called when the browser
            //   attempts to access the passive property.
	    passiveSupported = true;
	    return false;
	}
    };

    window.addEventListener("test", null, options);
    window.removeEventListener("test", null, options);
} catch(err) {
    passiveSupported = false;
}

/* This base64code is based on https://github.com/beatgammit/base64-js/blob/master/index.js which is MIT licensed */

var b64_lookup = [];
var base64_code = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/';
for (var i = 0, len = base64_code.length; i < len; ++i) {
    b64_lookup[i] = base64_code[i];
}

function tripletToBase64 (num) {
    return b64_lookup[num >> 18 & 0x3F] +
        b64_lookup[num >> 12 & 0x3F] +
        b64_lookup[num >> 6 & 0x3F] +
        b64_lookup[num & 0x3F];
}

function encodeBase64Chunk (uint8, start, end) {
    var tmp;
    var output = [];
    for (var i = start; i < end; i += 3) {
        tmp =
            ((uint8[i] << 16) & 0xFF0000) +
            ((uint8[i + 1] << 8) & 0xFF00) +
            (uint8[i + 2] & 0xFF);
        output.push(tripletToBase64(tmp));
    }
    return output.join('');
}

function bytesToDataUri(uint8) {
    var tmp;
    var len = uint8.length;
    var extraBytes = len % 3; // if we have 1 byte left, pad 2 bytes
    var parts = [];
    var maxChunkLength = 16383; // must be multiple of 3

    parts.push("data:image/png;base64,");

    // go through the array every three bytes, we'll deal with trailing stuff later
    for (var i = 0, len2 = len - extraBytes; i < len2; i += maxChunkLength) {
        parts.push(encodeBase64Chunk(uint8, i, (i + maxChunkLength) > len2 ? len2 : (i + maxChunkLength)));
    }

    // pad the end with zeros, but make sure to not forget the extra bytes
    if (extraBytes === 1) {
        tmp = uint8[len - 1];
        parts.push(b64_lookup[tmp >> 2] + b64_lookup[(tmp << 4) & 0x3F] + '==');
    } else if (extraBytes === 2) {
        tmp = (uint8[len - 2] << 8) + uint8[len - 1];
        parts.push(b64_lookup[tmp >> 10] + b64_lookup[(tmp >> 4) & 0x3F] + b64_lookup[(tmp << 2) & 0x3F] + '=');
    }

    return parts.join('');
}

/* Helper functions for debugging */
var logDiv = null;
function log(str) {
    if (!logDiv) {
        logDiv = document.createElement('pre');
        document.body.appendChild(logDiv);
        logDiv.style["position"] = "absolute";
        logDiv.style["right"] = "0px";
        logDiv.style["font-size"] = "small";
    }
    logDiv.insertBefore(document.createElement('br'), logDiv.firstChild);
    logDiv.insertBefore(document.createTextNode(str), logDiv.firstChild);
}

function getStackTrace()
{
    var callstack = [];
    var isCallstackPopulated = false;
    try {
        i.dont.exist+=0;
    } catch(e) {
        if (e.stack) { // Firefox
            var lines = e.stack.split("\n");
            for (var i=0, len=lines.length; i<len; i++) {
                if (lines[i].match(/^\s*[A-Za-z0-9\-_\$]+\(/)) {
                    callstack.push(lines[i]);
                }
            }
            // Remove call to getStackTrace()
            callstack.shift();
            isCallstackPopulated = true;
        } else if (window.opera && e.message) { // Opera
            var lines = e.message.split("\n");
            for (var i=0, len=lines.length; i<len; i++) {
                if (lines[i].match(/^\s*[A-Za-z0-9\-_\$]+\(/)) {
                    var entry = lines[i];
                    // Append next line also since it has the file info
                    if (lines[i+1]) {
                        entry += " at " + lines[i+1];
                        i++;
                    }
                    callstack.push(entry);
                }
            }
            // Remove call to getStackTrace()
            callstack.shift();
            isCallstackPopulated = true;
        }
    }
    if (!isCallstackPopulated) { //IE and Safari
        var currentFunction = arguments.callee.caller;
        while (currentFunction) {
            var fn = currentFunction.toString();
            var fname = fn.substring(fn.indexOf("function") + 8, fn.indexOf("(")) || "anonymous";
            callstack.push(fname);
            currentFunction = currentFunction.caller;
        }
    }
    return callstack;
}

function logStackTrace(len) {
    var callstack = getStackTrace();
    var end = callstack.length;
    if (len > 0)
        end = Math.min(len + 1, end);
    for (var i = 1; i < end; i++)
        log(callstack[i]);
}

/* Helper functions for touch identifier to make it unique on Android */
var globalTouchIdentifier = Math.round(Date.now() / 1000);
function touchIdentifierStart(tId)
{
    if (isAndroidChrome) {
	if (tId == 0) {
	    return ++globalTouchIdentifier;
	}
	return globalTouchIdentifier + tId;
    }
    return tId;
}
function touchIdentifier(tId)
{
    if (isAndroidChrome) {
	return globalTouchIdentifier + tId;
    }
    return tId;
}

var grab = new Object();
grab.surface = null;
grab.ownerEvents = false;
grab.implicit = false;
var keyDownList = [];
var inputList = [];
var lastSerial = 0;
var lastX = 0;
var lastY = 0;
var lastState;
var lastTimeStamp = 0;
var realSurfaceWithMouse = 0;
var surfaceWithMouse = 0;
var surfaces = {};
var textures = {};
var stackingOrder = [];
var outstandingCommands = new Array();
var outstandingDisplayCommands = null;
var inputSocket = null;
var debugDecoding = false;
var fakeInput = null;

/* Auto-reconnect + session liveness (see the block above connect()). */
var ws = null;
var sessionToken = null;       /* daemon id from BROADWAY_OP_SESSION; null until first seen */
var clientId = 0;              /* server-assigned id; sent back as ?cid= on reconnect */
var sessionInvalidated = false;/* true => give up, show disconnected until manual refresh */
var reconnecting = false;      /* true while the dim+spinner overlay is up */
var reconnectTimer = null;
var reconnectDelay = 0;        /* backoff in ms */
var awaitFirstFrame = false;   /* drop the overlay once the resync repaints */
var resumeSafetyTimer = null;  /* fallback to drop the overlay if no frame arrives */
var overlayEl = null;

/* Heartbeat: catches a half-open socket that never fires onclose (wi-fi off on
 * a foreground tab, cellular handover). Any inbound message counts as a PONG. */
var HEARTBEAT_MS = 2500;
var HEARTBEAT_TIMEOUT_MS = 7000;
var heartbeatTimer = null;
var lastPongTime = 0;
var pingSentTime = 0;          /* when the last PING went out, for RTT measurement */
var lastRtt = 0;               /* last measured round-trip (ms), reported to the daemon */
var paintFlashing = false;     /* debug overlay: received (magenta), reused (green), uploaded (red) */
const FLASH_MS = 100;          /* how long a flash stays up (no fade) */
var textureBatch = 0;          /* bumped per message; a Texture's batch == this => uploaded now */
var flashPool = [];            /* recycled paint-flash overlay divs (avoids per-node create/remove churn) */
const FLASH_MAX = 200;         /* cap overlays drawn per frame to keep layout cost bounded */
/* Clipboard paste capture: a hidden, focused textarea receives the browser's
 * native 'paste' event (Ctrl+V). Unlike navigator.clipboard.readText(), that
 * event needs no permission/user-activation popup, so we cache the pasted text
 * and answer the GTK app's clipboard request from it. */
var clipboardArea = null;
var pasteCache = "";
var pasteCacheTime = 0;
/* A 'keyPress' (or 'paste', or 'compositionend') and the trailing 'beforeinput'
 * the browser fires to mirror it are dispatched in the *same* event-loop turn,
 * so a small wall-clock window cleanly separates "this beforeinput just echoes
 * something we already forwarded" from "this is a genuinely new character". The
 * window only has to outlast that single turn; it stays far below the gap
 * between two real keystrokes, so it never swallows distinct input. The
 * keyPress/composition cases additionally match on the text itself (below), so
 * the window can at worst double a repeated char, never drop a different one. */
const INPUT_ECHO_WINDOW_MS = 50;
/* Dedupes a 'paste' against its trailing 'beforeinput' on touch. */
var lastPasteForwardTime = 0;
/* The text + time of the last real keyPress, to dedupe the trailing 'beforeinput'
 * on the touch OSK input (Latin keys fire both; IME/non-Latin only beforeinput). */
var lastKeyPressTime = 0;
var lastKeyPressText = "";
/* True while an IME composition is in progress on the touch OSK input. */
var imeComposing = false;
/* The text + time of the last compositionend, to dedupe a trailing 'insertText'
 * beforeinput that some browsers fire after the composition already committed. */
var lastCompositionEndTime = 0;
var lastCompositionText = "";
var showKeyboard = false;
var showKeyboardChanged = false;
var firstTouchDownId = null;
/* Surface id per active touch, captured at touchstart. A drag keeps routing to
 * the right surface even after GTK repaints and detaches the node it began on. */
var touchSurfaceIds = {};

/* Touch pinch-zoom. Mirrors desktop browser page-zoom: a two-finger pinch sets
 * zoomFactor, which (a) scales down the size we report to GTK and bumps the
 * reported scale (sendScreenSizeChanged) so GTK re-lays-out crisply, and (b)
 * magnifies the surface wrapper (#zoomRoot) by the same factor so it fills the
 * viewport. Page coords map to surface root coords by a plain divide-by-zoom
 * (getPositionsFromEvent), since the wrapper's transform-origin is 0 0. */
var ZOOM_MIN = 0.25;
var ZOOM_MAX = 5.0;
var zoomFactor = 1.0;       /* committed zoom; drives reported size + remap */
var zoomRoot = null;        /* the #zoomRoot wrapper element (or document.body) */
/* Live pinch state. activeTouches tracks every finger by identifier so we can
 * measure the pinch distance; suppressTouchForward stays true from pinch-begin
 * until ALL fingers lift, so a leftover finger never becomes a stray GTK tap. */
var activeTouches = {};
var pinchActive = false;
var pinchStartDist = 0;
var pinchStartZoom = 1.0;
var pinchStartMid = { x: 0, y: 0 };
var suppressTouchForward = false;

function clampZoom(z) {
    if (z < ZOOM_MIN) return ZOOM_MIN;
    if (z > ZOOM_MAX) return ZOOM_MAX;
    return z;
}

function touchPointDistance(a, b) {
    var dx = a.x - b.x, dy = a.y - b.y;
    return Math.sqrt(dx * dx + dy * dy);
}

/* The two-point distance of the currently active fingers (first two by id). */
function activeTouchDistance() {
    var pts = [];
    for (var k in activeTouches) {
        pts.push(activeTouches[k]);
        if (pts.length == 2)
            break;
    }
    if (pts.length < 2)
        return 0;
    return touchPointDistance(pts[0], pts[1]);
}

function activeTouchCount() {
    return Object.keys(activeTouches).length;
}

/* Midpoint (page coords) of the first two active fingers. */
function activeTouchMidpoint() {
    var pts = [];
    for (var k in activeTouches) {
        pts.push(activeTouches[k]);
        if (pts.length == 2)
            break;
    }
    if (pts.length == 0)
        return { x: 0, y: 0 };
    if (pts.length == 1)
        return { x: pts[0].x, y: pts[0].y };
    return { x: (pts[0].x + pts[1].x) / 2, y: (pts[0].y + pts[1].y) / 2 };
}

/* Magnify the surface wrapper. At z == 1 leave transform unset so the desktop
 * path is untouched (no extra compositing layer / text reblur). */
function applyZoomTransform(z) {
    if (zoomRoot == null)
        return;
    zoomRoot.style.transform = (z == 1.0) ? "" : ("scale(" + z + ")");
}

/* Live pinch preview: scale around the pinch midpoint and let it follow the
 * fingers, so the magnified view floats with them (Apple-like) instead of
 * anchoring to the top-left corner. We pin the layout point that was under the
 * start midpoint to the current midpoint: page = translate(t) + zoom * layout,
 * with layout_focus = pinchStartMid / pinchStartZoom (the pre-pinch transform
 * was scale(pinchStartZoom) at origin 0 0). On commit endPinch resets to a plain
 * scale (no translate) as the crisp reflow fills the viewport. */
function applyPinchPreview() {
    if (zoomRoot == null)
        return;
    var mid = activeTouchMidpoint();
    var tx = mid.x - zoomFactor * (pinchStartMid.x / pinchStartZoom);
    var ty = mid.y - zoomFactor * (pinchStartMid.y / pinchStartZoom);
    zoomRoot.style.transform = "translate(" + tx + "px," + ty + "px) scale(" + zoomFactor + ")";
}

/* Persist the committed zoom (per origin) so a page refresh (common on mobile)
 * keeps it instead of snapping back to 1. localStorage may be unavailable
 * (private mode / disabled), so guard. */
var ZOOM_STORAGE_KEY = "broadwayZoom";

function saveZoom() {
    try { window.localStorage.setItem(ZOOM_STORAGE_KEY, String(zoomFactor)); }
    catch (e) { }
}

function loadSavedZoom() {
    try {
        var v = parseFloat(window.localStorage.getItem(ZOOM_STORAGE_KEY));
        if (!isNaN(v))
            zoomFactor = clampZoom(v);
    } catch (e) { }
}

function getButtonMask (button) {
    if (button == 1)
        return GDK_BUTTON1_MASK;
    if (button == 2)
        return GDK_BUTTON2_MASK;
    if (button == 3)
        return GDK_BUTTON3_MASK;
    if (button == 4)
        return GDK_BUTTON4_MASK;
    if (button == 5)
        return GDK_BUTTON5_MASK;
    return 0;
}

function Texture(id, data) {
    var url;
    if (useDataUrls) {
        url = bytesToDataUri(data);
    } else {
        var blob = new Blob([data],{type: "image/png"});
        url = window.URL.createObjectURL(blob);
    }

    this.url = url;
    this.refcount = 1;
    this.id = id;
    this.batch = textureBatch;  /* which message uploaded these pixels (for paint-flash) */

    var image = new Image();
    image.src = this.url;
    this.image = image;
    this.decoded = image.decode();
    textures[id] = this;
}

Texture.prototype.ref = function() {
    this.refcount += 1;
    return this;
}

Texture.prototype.unref = function() {
    this.refcount -= 1;
    if (this.refcount == 0) {
        if (this.url.startsWith("blob")) {
            window.URL.revokeObjectURL(this.url);
        }
        delete textures[this.id];
    }
}

function sendConfigureNotify(surface)
{
    sendInput(BROADWAY_EVENT_CONFIGURE_NOTIFY, [surface.id, surface.x, surface.y, surface.width, surface.height]);
}

function cmdCreateSurface(id, x, y, width, height)
{
    var surface = { id: id, x: x, y:y, width: width, height: height };
    surface.transientParent = 0;
    surface.visible = false;
    surface.imageData = null;
    surface.nodes = {};

    var div = document.createElement('div');
    div.surface = surface;
    surface.div = div;

    div.style["position"] = "absolute";
    div.style["left"] = surface.x + "px";
    div.style["top"] = surface.y + "px";
    div.style["width"] = surface.width + "px";
    div.style["height"] = surface.height + "px";
    div.style["display"] = "block";
    div.style["visibility"] = "hidden";

    surfaces[id] = surface;
    stackingOrder.push(surface);

    sendConfigureNotify(surface);

    return div;
}

function restackSurfaces() {
    for (var i = 0; i < stackingOrder.length; i++) {
        var surface = stackingOrder[i];
        surface.div.style.zIndex = i;
    }
}

function moveToHelper(surface, position) {
    var i = stackingOrder.indexOf(surface);
    stackingOrder.splice(i, 1);
    if (position != undefined)
        stackingOrder.splice(position, 0, surface);
    else
        stackingOrder.push(surface);

    for (var cid in surfaces) {
        var child = surfaces[cid];
        if (child.transientParent == surface.id)
            moveToHelper(child, stackingOrder.indexOf(surface) + 1);
    }
}

function cmdRoundtrip(id, tag)
{
    sendInput(BROADWAY_EVENT_ROUNDTRIP_NOTIFY, [id, tag]);
}

function cmdRaiseSurface(id)
{
    var surface = surfaces[id];
    if (surface)
        moveToHelper(surface);
}

function cmdLowerSurface(id)
{
    var surface = surfaces[id];
    if (surface)
        moveToHelper(surface, 0);
}

function TransformNodes(node_data, div, nodes, display_commands) {
    this.node_data = node_data;
    this.display_commands = display_commands;
    this.data_pos = 0;
    this.div = div;
    this.outstanding = 1;
    this.nodes = nodes;
}

TransformNodes.prototype.decode_uint32 = function() {
    var v = this.node_data.getUint32(this.data_pos, true);
    this.data_pos += 4;
    return v;
}

TransformNodes.prototype.decode_int32 = function() {
    var v = this.node_data.getInt32(this.data_pos, true);
    this.data_pos += 4;
    return v;
}

TransformNodes.prototype.decode_float = function() {
    var v = this.node_data.getFloat32(this.data_pos, true);
    this.data_pos += 4;
    return v;
}

TransformNodes.prototype.decode_color = function() {
    var rgba = this.decode_uint32();
    var a = (rgba >> 24) & 0xff;
    var r = (rgba >> 16) & 0xff;
    var g = (rgba >> 8) & 0xff;
    var b = (rgba >> 0) & 0xff;
    var c;
    if (a == 255)
        c = "rgb(" + r + "," + g + "," + b + ")";
    else
        c = "rgba(" + r + "," + g + "," + b + "," + (a / 255.0) + ")";
    return c;
}

TransformNodes.prototype.decode_size = function() {
    var s = new Object();
    s.width = this.decode_float ();
    s.height = this.decode_float ();
    return s;
}

TransformNodes.prototype.decode_point = function() {
    var p = new Object();
    p.x = this.decode_float ();
    p.y = this.decode_float ();
    return p;
}

TransformNodes.prototype.decode_rect = function() {
    var r = new Object();
    r.x = this.decode_float ();
    r.y = this.decode_float ();
    r.width = this.decode_float ();
    r.height = this.decode_float ();
    return r;
}

TransformNodes.prototype.decode_irect = function() {
    var r = new Object();
    r.x = this.decode_int32 ();
    r.y = this.decode_int32 ();
    r.width = this.decode_int32 ();
    r.height = this.decode_int32 ();
    return r;
}

TransformNodes.prototype.decode_rounded_rect = function() {
    var r = new Object();
    r.bounds = this.decode_rect();
    r.sizes = [];
    for (var i = 0; i < 4; i++)
        r.sizes[i] = this.decode_size();
    return r;
}

TransformNodes.prototype.decode_color_stop = function() {
    var s = new Object();
    s.offset = this.decode_float ();
    s.color = this.decode_color ();
    return s;
}

TransformNodes.prototype.decode_color_stops = function() {
    var stops = [];
    var len = this.decode_uint32();
    for (var i = 0; i < len; i++)
        stops[i] = this.decode_color_stop();
    return stops;
}

function utf8_to_string(array) {
    var out, i, len, c;
    var char2, char3;

    out = "";
    len = array.length;
    i = 0;
    while(i < len) {
    c = array[i++];
    switch(c >> 4)
        {
        case 0: case 1: case 2: case 3: case 4: case 5: case 6: case 7:
            // 0xxxxxxx
            out += String.fromCharCode(c);
            break;
        case 12: case 13:
            // 110x xxxx   10xx xxxx
            char2 = array[i++];
            out += String.fromCharCode(((c & 0x1F) << 6) | (char2 & 0x3F));
            break;
        case 14:
            // 1110 xxxx  10xx xxxx  10xx xxxx
            char2 = array[i++];
            char3 = array[i++];
            out += String.fromCharCode(((c & 0x0F) << 12) |
                                       ((char2 & 0x3F) << 6) |
                                       ((char3 & 0x3F) << 0));
            break;
        }
    }

    return out;
}

TransformNodes.prototype.decode_string = function() {
    var len = this.decode_uint32();
    var utf8 = new Array();
    var b;
    for (var i = 0; i < len; i++) {
        if (i % 4 == 0) {
            b = this.decode_uint32();
        }
        utf8[i] = b & 0xff;
        b = b >> 8;
    }

    return utf8_to_string (utf8);
}

TransformNodes.prototype.decode_transform = function() {
    var transform_type = this.decode_uint32();

    if (transform_type == 0) {
        var point = this.decode_point();
        return "translate(" + px(point.x) + "," + px(point.y) + ")";
    } else if (transform_type == 1) {
        var m = new Array();
        for (var i = 0; i < 16; i++) {
            m[i] = this.decode_float ();
        }

        return "matrix3d(" +
            m[0] + "," + m[1] + "," + m[2] + "," + m[3]+ "," +
            m[4] + "," + m[5] + "," + m[6] + "," + m[7] + "," +
            m[8] + "," + m[9] + "," + m[10] + "," + m[11] + "," +
            m[12] + "," + m[13] + "," + m[14] + "," + m[15] + ")";
    } else {
        alert("Unexpected transform type " + transform_type);
    }
}

function args() {
    var argsLength = arguments.length;
    var strings = [];
    for (var i = 0; i < argsLength; ++i)
        strings[i] = arguments[i];

    return strings.join(" ");
}

function px(x) {
    return x + "px";
}

function set_point_style (div, point) {
    div.style["left"] = px(point.x);
    div.style["top"] = px(point.y);
}

function set_rect_style (div, rect) {
    div.style["left"] = px(rect.x);
    div.style["top"] = px(rect.y);
    div.style["width"] = px(rect.width);
    div.style["height"] = px(rect.height);
}

function set_rrect_style (div, rrect) {
    set_rect_style(div, rrect.bounds);
    div.style["border-top-left-radius"] = args(px(rrect.sizes[0].width), px(rrect.sizes[0].height));
    div.style["border-top-right-radius"] = args(px(rrect.sizes[1].width), px(rrect.sizes[1].height));
    div.style["border-bottom-right-radius"] = args(px(rrect.sizes[2].width), px(rrect.sizes[2].height));
    div.style["border-bottom-left-radius"] = args(px(rrect.sizes[3].width), px(rrect.sizes[3].height));
}

TransformNodes.prototype.createDiv = function(id)
{
    var div = document.createElement('div');
    div.node_id = id;
    this.nodes[id] = div;
    return div;
}

TransformNodes.prototype.createImage = function(id)
{
    var image = new Image();
    image.node_id = id;
    this.nodes[id] = image;
    return image;
}

TransformNodes.prototype.insertNode = function(parent, previousSibling, is_toplevel)
{
    var type = this.decode_uint32();
    var id = this.decode_uint32();
    var newNode = null;
    var oldNode = null;

    switch (type)
    {
        /* Reuse divs from last frame */
        case BROADWAY_NODE_REUSE:
        {
            oldNode = this.nodes[id];
            /* Reused content node (kept/re-parented, not redrawn) -> "reused" colour. */
            if (oldNode && oldNode.__content)
                oldNode.__flashKind = 2;
        }
        break;
        /* Leaf nodes */

        case BROADWAY_NODE_TEXTURE:
        {
            var rect = this.decode_rect();
            var texture_id = this.decode_uint32();
            var image = this.createImage(id);
            image.width = rect.width;
            image.height = rect.height;
            image.style["position"] = "absolute";
            set_rect_style(image, rect);
            /* A texture id can be stale if it was released before this node is
             * processed (a lifecycle race; the daemon then remaps it to 0). Skip
             * gracefully - throwing here would wedge the whole render loop until a
             * page refresh. Mirrors the unknown-node handling in execute(). */
            var texture = textures[texture_id];
            if (texture) {
                texture.ref();
                image.src = texture.url;
                // Unref blob url when loaded
                image.onload = function() { texture.unref(); };
                image.__content = true;  /* drawn pixels */
                /* 3 = texture uploaded this batch (real traffic), 1 = node re-sent but
                 * the texture was cached (cheap). */
                image.__flashKind = (texture.batch === textureBatch) ? 3 : 1;
            } else {
                console.warn("broadway: NODE_TEXTURE references unknown texture " + texture_id);
            }
            newNode = image;
        }
        break;

    case BROADWAY_NODE_COLOR:
        {
            var rect = this.decode_rect();
            var c = this.decode_color ();
            var div = this.createDiv(id);
            div.style["position"] = "absolute";
            set_rect_style(div, rect);
            div.style["background-color"] = c;
            div.__content = true;
            div.__flashKind = 1;
            newNode = div;
        }
        break;

    case BROADWAY_NODE_BORDER:
        {
            var rrect = this.decode_rounded_rect();
            var border_widths = [];
            for (var i = 0; i < 4; i++)
                border_widths[i] = this.decode_float();
            var border_colors = [];
            for (var i = 0; i < 4; i++)
                border_colors[i] = this.decode_color();

            var div = this.createDiv(id);
            div.style["position"] = "absolute";

            /* Draw a border as inset box-shadow(s) on one element, like the
             * window frame's 1px border. A CSS border is a separate element
             * whose edge meets the background's, which the browser rasterizes
             * with a 1px seam (and a corner sliver) at any zoom. Uniform border:
             * coincide this div with the background sibling (border box for
             * buttons, 1px smaller for menus) and use one inset shadow. */
            if (border_widths[0] === border_widths[1] &&
                border_widths[1] === border_widths[2] &&
                border_widths[2] === border_widths[3] &&
                border_colors[0] === border_colors[1] &&
                border_colors[1] === border_colors[2] &&
                border_colors[2] === border_colors[3]) {
                var bg = previousSibling;
                if (bg && bg.tagName === "DIV" && bg.style.width &&
                    Math.abs(parseFloat(bg.style.left) - rrect.bounds.x) <= border_widths[0] + 1 &&
                    Math.abs(parseFloat(bg.style.top) - rrect.bounds.y) <= border_widths[0] + 1) {
                    div.style["left"] = bg.style.left;
                    div.style["top"] = bg.style.top;
                    div.style["width"] = bg.style.width;
                    div.style["height"] = bg.style.height;
                    div.style["border-top-left-radius"] = bg.style.borderTopLeftRadius;
                    div.style["border-top-right-radius"] = bg.style.borderTopRightRadius;
                    div.style["border-bottom-right-radius"] = bg.style.borderBottomRightRadius;
                    div.style["border-bottom-left-radius"] = bg.style.borderBottomLeftRadius;
                } else {
                    set_rrect_style(div, rrect);
                }
                div.style["box-shadow"] = "inset 0 0 0 " + px(border_widths[0]) + " " + border_colors[0];
            } else {
                /* Per-side border (an active button's darker top edge): one
                 * directional inset shadow per side, same single-element trick. */
                set_rrect_style(div, rrect);
                div.style["box-shadow"] =
                    "inset 0 " + px(border_widths[0]) + " 0 0 " + border_colors[0] + ", " +
                    "inset " + px(-border_widths[1]) + " 0 0 0 " + border_colors[1] + ", " +
                    "inset 0 " + px(-border_widths[2]) + " 0 0 " + border_colors[2] + ", " +
                    "inset " + px(border_widths[3]) + " 0 0 0 " + border_colors[3];
            }
            newNode = div;
        }
        break;

    case BROADWAY_NODE_OUTSET_SHADOW:
        {
            var rrect = this.decode_rounded_rect();
            var color = this.decode_color();
            var dx = this.decode_float();
            var dy = this.decode_float();
            var spread = this.decode_float();
            var blur = this.decode_float();

            var div = this.createDiv(id);
            div.style["position"] = "absolute";
            set_rrect_style(div, rrect);
            div.style["box-shadow"] = args(px(dx), px(dy), px(blur), px(spread), color);
            newNode = div;
        }
        break;

    case BROADWAY_NODE_INSET_SHADOW:
        {
            var rrect = this.decode_rounded_rect();
            var color = this.decode_color();
            var dx = this.decode_float();
            var dy = this.decode_float();
            var spread = this.decode_float();
            var blur = this.decode_float();

            var div = this.createDiv(id);
            div.style["position"] = "absolute";
            set_rrect_style(div, rrect);
            div.style["box-shadow"] = args("inset", px(dx), px(dy), px(blur), px(spread), color);
            newNode = div;
        }
        break;


    case BROADWAY_NODE_LINEAR_GRADIENT:
        {
            var rect = this.decode_rect();
            var start = this.decode_point ();
            var end = this.decode_point ();
            var stops = this.decode_color_stops ();
            var div = this.createDiv(id);
            div.style["position"] = "absolute";
            set_rect_style(div, rect);

            // direction:
            var dx = end.x - start.x;
            var dy = end.y - start.y;

            // Angle in css coords (clockwise degrees, up = 0), note that y goes downwards so we have to invert
            var angle = Math.atan2(dx, -dy) * 180.0 / Math.PI;

            // Figure out which corner has offset == 0 in css
            var start_corner_x, start_corner_y;
            if (dx >= 0) // going right
                start_corner_x = rect.x;
            else
                start_corner_x = rect.x + rect.width;
            if (dy >= 0) // going down
                start_corner_y = rect.y;
            else
                start_corner_y = rect.y + rect.height;

            /* project start corner on the line */
            var l2 = dx*dx + dy*dy;
            var l = Math.sqrt(l2);
            var offset = ((start_corner_x - start.x) * dx  + (start_corner_y - start.y) * dy) / l2;

            var gradient = "linear-gradient(" + angle + "deg";
            for (var i = 0; i < stops.length; i++) {
                var stop = stops[i];
                gradient = gradient + ", " + stop.color + " " + px(stop.offset * l - offset);
            }
            gradient = gradient + ")";

            div.style["background-image"] = gradient;
            newNode = div;
        }
        break;


    /* Bin nodes */

    case BROADWAY_NODE_TRANSFORM:
        {
            var transform_string = this.decode_transform();

            var div = this.createDiv(id);
            div.style["transform"] = transform_string;
            div.style["transform-origin"] = "0px 0px";

            this.insertNode(div, null, false);
            newNode = div;
        }
        break;

    case BROADWAY_NODE_CLIP:
        {
            var rect = this.decode_rect();
            var div = this.createDiv(id);
            div.style["position"] = "absolute";
            set_rect_style(div, rect);
            div.style["overflow"] = "hidden";
            this.insertNode(div, null, false);
            newNode = div;
        }
        break;

    case BROADWAY_NODE_ROUNDED_CLIP:
        {
            var rrect = this.decode_rounded_rect();
            var div = this.createDiv(id);
            div.style["position"] = "absolute";
            set_rrect_style(div, rrect);
            div.style["overflow"] = "hidden";
            this.insertNode(div, null, false);
            newNode = div;
        }
        break;

    case BROADWAY_NODE_OPACITY:
        {
            var opacity = this.decode_float();
            var div = this.createDiv(id);
            div.style["position"] = "absolute";
            div.style["left"] = px(0);
            div.style["top"] = px(0);
            div.style["opacity"] = opacity;

            this.insertNode(div, null, false);
            newNode = div;
        }
        break;

    case BROADWAY_NODE_SHADOW:
        {
            var len = this.decode_uint32();
            var filters = "";
            for (var i = 0; i < len; i++) {
                var color = this.decode_color();
                var dx = this.decode_float();
                var dy = this.decode_float();
                var blur = this.decode_float();
                filters = filters + "drop-shadow(" + args (px(dx), px(dy), px(blur), color) + ")";
            }
            var div = this.createDiv(id);
            div.style["position"] = "absolute";
            div.style["left"] = px(0);
            div.style["top"] = px(0);
            div.style["filter"] = filters;

            this.insertNode(div, null, false);
            newNode = div;
        }
        break;

    case BROADWAY_NODE_DEBUG:
        {
            var str = this.decode_string();
            var div = this.createDiv(id);
            div.setAttribute('debug', str);
            this.insertNode(div, null, false);
            newNode = div;
        }
        break;

   /* Generic nodes */

    case BROADWAY_NODE_CONTAINER:
        {
            var div = this.createDiv(id);
            var len = this.decode_uint32();
            var lastChild = null;
            for (var i = 0; i < len; i++) {
                lastChild = this.insertNode(div, lastChild, false);
            }
            newNode = div;
        }
        break;

    default:
        alert("Unexpected node type " + type);
    }

    if (newNode) {
        if (is_toplevel)
            this.display_commands.push([DISPLAY_OP_INSERT_AFTER_CHILD, parent, previousSibling, newNode]);
        else // It is safe to display directly because we have not added the toplevel to the document yet
            parent.appendChild(newNode);
        return newNode;
    } else if (oldNode) {
        // This must be delayed until display ops, because it will remove from the old parent
        this.display_commands.push([DISPLAY_OP_INSERT_AFTER_CHILD, parent, previousSibling, oldNode]);
        return oldNode;
    }
}

TransformNodes.prototype.execute = function(display_commands)
{
    var root = this.div;

    while (this.data_pos < this.node_data.byteLength) {
        var op = this.decode_uint32();
        var parentId, parent;

        switch (op) {
        case BROADWAY_NODE_OP_INSERT_NODE:
            parentId = this.decode_uint32();
            if (parentId == 0)
                parent = root;
            else {
                parent = this.nodes[parentId];
                if (parent == null)
                    console.log("Wanted to insert into parent " + parentId + " but it is unknown");
            }

            var previousChildId = this.decode_uint32();
            var previousChild = null;
            if (previousChildId != 0)
                previousChild = this.nodes[previousChildId];
            this.insertNode(parent, previousChild, true);
            break;
        case BROADWAY_NODE_OP_REMOVE_NODE:
            var removeId = this.decode_uint32();
            var remove = this.nodes[removeId];
            delete this.nodes[removeId];
            if (remove == null)
                console.log("Wanted to delete node " + removeId + " but it is unknown");
            else
                this.display_commands.push([DISPLAY_OP_DELETE_NODE, remove]);
            break;
        case BROADWAY_NODE_OP_MOVE_AFTER_CHILD:
            parentId = this.decode_uint32();
            if (parentId == 0)
                parent = root;
            else
                parent = this.nodes[parentId];
            var previousChildId = this.decode_uint32();
            var previousChild = null;
            if (previousChildId != 0)
                previousChild = this.nodes[previousChildId];
            var toMoveId = this.decode_uint32();
            var toMove = this.nodes[toMoveId];
            this.display_commands.push([DISPLAY_OP_INSERT_AFTER_CHILD, parent, previousChild, toMove]);
            break;
        case BROADWAY_NODE_OP_PATCH_TEXTURE:
            var textureNodeId = this.decode_uint32();
            var textureNode = this.nodes[textureNodeId];
            var textureId = this.decode_uint32();
            var texture = textures[textureId];
            if (texture) {
                texture.ref();
                this.display_commands.push([DISPLAY_OP_CHANGE_TEXTURE, textureNode, texture]);
            } else {
                console.warn("broadway: PATCH_TEXTURE references unknown texture " + textureId);
            }
            break;
        case BROADWAY_NODE_OP_PATCH_TRANSFORM:
            var transformNodeId = this.decode_uint32();
            var transformNode = this.nodes[transformNodeId];
            var transformString = this.decode_transform();
            this.display_commands.push([DISPLAY_OP_CHANGE_TRANSFORM, transformNode, transformString]);
            break;
        }

    }
}

function cmdGrabPointer(id, ownerEvents)
{
    doGrab(id, ownerEvents, false);
    sendInput (BROADWAY_EVENT_GRAB_NOTIFY, []);
}

function cmdUngrabPointer()
{
    sendInput (BROADWAY_EVENT_UNGRAB_NOTIFY, []);
    if (grab.surface)
        doUngrab();
}

/* A pooled paint-flash overlay div. Created once and kept in document.body
 * forever (just toggled display:none); recycled via flashPool so heavy frames
 * don't churn the DOM. */
function getFlashDiv() {
    var f = flashPool.pop();
    if (!f) {
        f = document.createElement("div");
        f.style.cssText = "position:fixed;pointer-events:none;z-index:2147483647;";
        document.body.appendChild(f);
    }
    return f;
}

function handleDisplayCommands(display_commands)
{
    /* A double-scheduled requestAnimationFrame can fire after a prior frame
     * already applied and cleared outstandingDisplayCommands, so the arg is
     * null - nothing left to apply. */
    if (!display_commands)
        return;
    var div, parent;
    var flashed = paintFlashing ? [] : null;
    var len = display_commands.length;
    for (var i = 0; i < len; i++) {
        var cmd = display_commands[i];

        /* A node/texture id can be stale (a lifecycle race during heavy scroll
         * leaves the renderer referencing a node the browser no longer has).
         * Guard each deref and keep a per-command try/catch backstop so one bad
         * command degrades gracefully instead of wedging the render loop until a
         * page refresh. */
        try {
        switch (cmd[0]) {
        case DISPLAY_OP_REPLACE_CHILD:
            if (cmd[1] && cmd[2] && cmd[3]) {
                cmd[1].replaceChild(cmd[2], cmd[3]);
                if (flashed && cmd[2].__flashKind) flashed.push(cmd[2]);
            }
            break;
        case DISPLAY_OP_APPEND_CHILD:
            if (cmd[1] && cmd[2]) {
                cmd[1].appendChild(cmd[2]);
                if (flashed && cmd[2].__flashKind) flashed.push(cmd[2]);
            }
            break;
        case DISPLAY_OP_INSERT_AFTER_CHILD:
            parent = cmd[1];
            var afterThis = cmd[2];
            div = cmd[3];
            if (parent && div) {
                if (afterThis == null) // First
                    parent.insertBefore(div, parent.firstChild);
                else
                    parent.insertBefore(div, afterThis.nextSibling);
                if (flashed && div.__flashKind) flashed.push(div);
            }
            break;
        case DISPLAY_OP_APPEND_ROOT:
            /* Into the zoom wrapper, not document.body, so the pinch-zoom
             * transform magnifies all surfaces (the offscreen clipboard/OSK
             * helpers stay on document.body and unscaled). */
            if (cmd[1])
                (zoomRoot || document.body).appendChild(cmd[1]);
            break;
        case DISPLAY_OP_SHOW_SURFACE:
            div = cmd[1];
            if (div) {
                div.style["left"] = cmd[2] + "px";
                div.style["top"] = cmd[3] + "px";
                div.style["visibility"] = "visible";
            }
            break;
        case DISPLAY_OP_HIDE_SURFACE:
            div = cmd[1];
            if (div)
                div.style["visibility"] = "hidden";
            break;
        case DISPLAY_OP_DELETE_NODE:
            div = cmd[1];
            if (div && div.parentNode)
                div.parentNode.removeChild(div);
            break;
        case DISPLAY_OP_MOVE_NODE:
            div = cmd[1];
            if (div) {
                div.style["left"] = cmd[2] + "px";
                div.style["top"] = cmd[3] + "px";
            }
            break;
        case DISPLAY_OP_RESIZE_NODE:
            div = cmd[1];
            if (div) {
                div.style["width"] = cmd[2] + "px";
                div.style["height"] = cmd[3] + "px";
            }
            break;

        case DISPLAY_OP_RESTACK_SURFACES:
            restackSurfaces();
            break;
        case DISPLAY_OP_DELETE_SURFACE:
            var id = cmd[1];
	    if (id == surfaceWithMouse) {
		surfaceWithMouse = 0;
	    }
	    if (id == realSurfaceWithMouse) {
		realSurfaceWithMouse = 0;
		firstTouchDownId = null;
	    }
           delete surfaces[id];
            break;
        case DISPLAY_OP_CHANGE_TEXTURE:
            var image = cmd[1];
            var texture = cmd[2];
            if (image && texture) {
                // We need a new closure here to have a separate copy of "texture" for each iteration in the onload callback...
                var block = function(t) {
                    image.src = t.url;
                    // Unref blob url when loaded
                    image.onload = function() { t.unref(); };
                };
                block(texture);
                if (flashed) { image.__content = true; image.__flashKind = (texture.batch === textureBatch) ? 3 : 1; flashed.push(image); }
            }
            break;
        case DISPLAY_OP_CHANGE_TRANSFORM:
            var div = cmd[1];
            if (div)
                div.style["transform"] = cmd[2];
            break;
        default:
            console.warn("broadway: unknown display op " + cmd[0]);
        }
        } catch (e) {
            console.warn("broadway: display op " + (cmd && cmd[0]) + " failed:", e);
        }
    }

    /* Flash after the whole batch is applied so geometry is settled. Read all
     * geometry first (one batched layout), then write all overlays, then a
     * single timer recycles this frame's divs - the old per-node
     * getBoundingClientRect + appendChild + setTimeout thrashed layout and
     * hung the page under heavy scrolling. Colour by kind:
     *   1 = received (node re-sent, texture cached) = magenta;
     *   2 = reused (kept node + texture, just re-parented) = green;
     *   3 = received + texture uploaded this frame (real traffic) = red. */
    if (flashed) {
        var rects = [];
        var drawN = Math.min(flashed.length, FLASH_MAX);
        for (var fi = 0; fi < drawN; fi++) {
            var el = flashed[fi];
            if (el && el.getBoundingClientRect) {
                var r = el.getBoundingClientRect();
                if (r.width >= 1 && r.height >= 1)
                    rects.push({ r: r, kind: el.__flashKind });
            }
        }
        for (var fr = 0; fr < flashed.length; fr++)   /* reset all, incl. uncapped */
            flashed[fr].__flashKind = 0;

        var batch = [];
        for (var dw = 0; dw < rects.length; dw++) {
            var d = rects[dw];
            var f = getFlashDiv();
            f.style.left = d.r.left + "px";
            f.style.top = d.r.top + "px";
            f.style.width = d.r.width + "px";
            f.style.height = d.r.height + "px";
            f.style.background = d.kind === 2 ? "rgba(0,200,0,0.45)"
                              : d.kind === 3 ? "rgba(255,0,0,0.65)"
                              : "rgba(255,0,200,0.5)";
            f.style.display = "block";
            batch.push(f);
        }
        if (batch.length)
            setTimeout(function () {
                for (var k = 0; k < batch.length; k++) {
                    batch[k].style.display = "none";
                    flashPool.push(batch[k]);
                }
            }, FLASH_MS);
    }
}

function handleCommands(cmd, display_commands, new_textures, modified_trees)
{
    var res = true;
    var need_restack = false;
    textureBatch++;   /* textures uploaded in this message are "fresh" (real traffic) */

    while (res && cmd.pos < cmd.length) {
        var id, x, y, w, h, q, surface;
        var saved_pos = cmd.pos;
        var command = cmd.get_uint8();
        lastSerial = cmd.get_32();
        switch (command) {
        case BROADWAY_OP_SESSION:
            var token = cmd.get_32();
            var cid = cmd.get_32();
            onSessionToken(token, cid);
            break;

        case BROADWAY_OP_PONG:
            /* handleMessage already bumped lastPongTime. Measure the round-trip so
             * the next PING reports it to the daemon's debug menu. */
            if (pingSentTime)
                lastRtt = Date.now() - pingSentTime;
            break;

        case BROADWAY_OP_DEBUG_FLASH:
            paintFlashing = cmd.get_uint8() != 0;
            break;

        case BROADWAY_OP_DISCONNECTED:
            /* Another client took the display: unrecoverable. */
            invalidateSession();
            break;

        case BROADWAY_OP_NEW_SURFACE:
            id = cmd.get_16();
            x = cmd.get_16s();
            y = cmd.get_16s();
            w = cmd.get_16();
            h = cmd.get_16();
            var div = cmdCreateSurface(id, x, y, w, h);
            display_commands.push([DISPLAY_OP_APPEND_ROOT, div]);
            need_restack = true;
            break;

        case BROADWAY_OP_SHOW_SURFACE:
            id = cmd.get_16();
            surface = surfaces[id];
            if (!surface.visible) {
                surface.visible = true;
                display_commands.push([DISPLAY_OP_SHOW_SURFACE, surface.div, surface.x, surface.y]);
                need_restack = true;
            }
           break;

        case BROADWAY_OP_HIDE_SURFACE:
            id = cmd.get_16();
            if (grab.surface == id)
                doUngrab();
            surface = surfaces[id];
            if (surface.visible) {
                surface.visible = false;
                display_commands.push([DISPLAY_OP_HIDE_SURFACE, surface.div]);
            }
            break;

        case BROADWAY_OP_SET_INPUT_REGION:
            id = cmd.get_16();
            var irMode = cmd.get_16();   /* 0 whole, 1 empty, 2 rect */
            /* x/y are signed: a toplevel's resize border sits just outside the
             * surface (negative), so read them as get_16s, not get_16. */
            var irX = cmd.get_16s(), irY = cmd.get_16s(), irW = cmd.get_16(), irH = cmd.get_16();
            surface = surfaces[id];
            if (surface)
                setInputRegion(surface, irMode, irX, irY, irW, irH);
            break;

        case BROADWAY_OP_REASSERT_POINTER:
            id = cmd.get_16();
            var rx = cmd.get_16();
            var ry = cmd.get_16();
            /* The daemon asks us to re-assert pointer focus after a popup that
             * held the (touch-emulated) pointer focus went away. Routing it
             * through the normal input path (rather than injecting it daemon-
             * side) gives the app's GTK a crossing with the live serial/time it
             * needs to actually move pointer focus, unsticking touch input.
             * The ENTER rebuilds the (stale) pointer focus; the immediately
             * following LEAVE clears the mouse-hover state it would otherwise
             * leave behind (spurious :hover styling / tooltips on touch). */
            sendInput(BROADWAY_EVENT_ENTER, [id, id, rx, ry, rx, ry, lastState, GDK_CROSSING_NORMAL]);
            sendInput(BROADWAY_EVENT_LEAVE, [id, id, rx, ry, rx, ry, lastState, GDK_CROSSING_NORMAL]);
            break;

        case BROADWAY_OP_SET_TRANSIENT_FOR:
            id = cmd.get_16();
            var parentId = cmd.get_16();
            surface = surfaces[id];
            if (surface.transientParent !== parentId) {
                surface.transientParent = parentId;
                if (parentId != 0 && surfaces[parentId]) {
                    moveToHelper(surface, stackingOrder.indexOf(surfaces[parentId])+1);
                }
                need_restack = true;
            }
            break;

        case BROADWAY_OP_DESTROY_SURFACE:
            id = cmd.get_16();

            if (grab.surface == id)
                doUngrab();

            surface = surfaces[id];
            var i = stackingOrder.indexOf(surface);
            if (i >= 0)
                stackingOrder.splice(i, 1);
            var div = surface.div;

            display_commands.push([DISPLAY_OP_DELETE_NODE, div]);
            // We need to delay this until its really deleted because we can still get events to it
            display_commands.push([DISPLAY_OP_DELETE_SURFACE, id]);
            break;

        case BROADWAY_OP_ROUNDTRIP:
            id = cmd.get_16();
            var tag = cmd.get_32();
            cmdRoundtrip(id, tag);
            break;

        case BROADWAY_OP_MOVE_RESIZE:
            id = cmd.get_16();
            var ops = cmd.get_flags();
            var has_pos = ops & 1;
            var has_size = ops & 2;
            surface = surfaces[id];
            if (has_pos) {
                surface.x = cmd.get_16s();
                surface.y = cmd.get_16s();
                display_commands.push([DISPLAY_OP_MOVE_NODE, surface.div, surface.x, surface.y]);
            }
            if (has_size) {
                surface.width = cmd.get_16();
                surface.height = cmd.get_16();
                display_commands.push([DISPLAY_OP_RESIZE_NODE, surface.div, surface.width, surface.height]);

            }
            sendConfigureNotify(surface);
            break;

        case BROADWAY_OP_RAISE_SURFACE:
            id = cmd.get_16();
            cmdRaiseSurface(id);
            need_restack = true;
            break;

        case BROADWAY_OP_LOWER_SURFACE:
            id = cmd.get_16();
            cmdLowerSurface(id);
            need_restack = true;
            break;

        case BROADWAY_OP_UPLOAD_TEXTURE:
            id = cmd.get_32();
            var data = cmd.get_data();
            var texture = new Texture (id, data); // Stores a ref in global textures array
            new_textures.push(texture);
            break;

        case BROADWAY_OP_RELEASE_TEXTURE:
            id = cmd.get_32();
            textures[id].unref();
            break;

        case BROADWAY_OP_SET_NODES:
            id = cmd.get_16();
            if (id in modified_trees) {
                // Can't modify the same dom tree in the same loop, bail out and do the first one
                cmd.pos = saved_pos;
                res = false;
            } else {
                modified_trees[id] = true;

                var node_data = cmd.get_nodes ();
                surface = surfaces[id];
                var transform_nodes = new TransformNodes (node_data, surface.div, surface.nodes, display_commands);
                transform_nodes.execute();
            }
            break;

        case BROADWAY_OP_GRAB_POINTER:
            id = cmd.get_16();
            var ownerEvents = cmd.get_bool ();

            cmdGrabPointer(id, ownerEvents);
            break;

        case BROADWAY_OP_UNGRAB_POINTER:
            cmdUngrabPointer();
            break;

        case BROADWAY_OP_SET_SHOW_KEYBOARD:
            var wantKeyboard = cmd.get_16() != 0;
            if (wantKeyboard != showKeyboard) {
                showKeyboard = wantKeyboard;
                showKeyboardChanged = true;
                /* GTK requests this within ~10-25ms of the tap that moved focus,
                 * so the tap's transient activation is still live and focus()
                 * can summon the keyboard now - no need to wait for the next
                 * touch (which made the OSK lag a gesture behind). */
                applyKeyboard();
            }
            break;

        case BROADWAY_OP_SET_CLIPBOARD:
            var _cbdata = cmd.get_data();
            var _cbtext = new TextDecoder("utf-8").decode(_cbdata);
            setHostClipboard(_cbtext);
            break;

        case BROADWAY_OP_OPEN_URI:
            /* No system URI handler in a headless Broadway session, so the app
             * routes link clicks here. Open them in a new browser tab. The
             * call runs inside the WebSocket message handler (not a user
             * gesture), so a popup blocker may catch it; 'noopener' keeps the
             * new tab from reaching back into this page. */
            var _uridata = cmd.get_data();
            var _uri = new TextDecoder("utf-8").decode(_uridata);
            window.open(_uri, "_blank", "noopener");
            break;

        case BROADWAY_OP_REQUEST_CLIPBOARD:
            /* IIFE so each request's id is captured by the async callbacks even
             * if several requests arrive in one command batch. */
            (function (id) {
                if (Date.now() - pasteCacheTime < 1000) {
                    /* A native paste event (Ctrl+V) just captured the clipboard:
                     * answer from it with no permission popup. Consume it so a
                     * later request can't reuse stale text. */
                    pasteCacheTime = 0;
                    sendClipboardContents(pasteCache, id);
                } else if (navigator.clipboard && navigator.clipboard.readText) {
                    /* No recent native paste (e.g. the right-click menu's Paste):
                     * fall back to the async API. Silent on Chromium once
                     * permission is granted; Firefox shows its one-time prompt. */
                    navigator.clipboard.readText().then(
                        function (t) { sendClipboardContents(t, id); },
                        function (e) { sendClipboardContents("", id); });
                } else {
                    sendClipboardContents("", id);
                }
            })(cmd.get_32());
            break;

        default:
            alert("Unknown op " + command);
        }
    }

    if (need_restack)
        display_commands.push([DISPLAY_OP_RESTACK_SURFACES]);

    return res;
}

function handleOutstandingDisplayCommands()
{
    if (outstandingDisplayCommands) {
        window.requestAnimationFrame(
            function () {
                try {
                    handleDisplayCommands(outstandingDisplayCommands);
                } catch (e) {
                    console.warn("broadway: handleDisplayCommands failed:", e);
                }
                outstandingDisplayCommands = null;

                /* First repaint after a resume: drop the dim+spinner overlay. */
                if (awaitFirstFrame) {
                    awaitFirstFrame = false;
                    reconnecting = false;
                    if (resumeSafetyTimer) {
                        clearTimeout(resumeSafetyTimer);
                        resumeSafetyTimer = null;
                    }
                    hideOverlay();
                }

                if (outstandingCommands.length > 0)
                    setTimeout(handleOutstanding);
            });
    } else {
        if (outstandingCommands.length > 0)
            handleOutstanding ();
    }
}

/* Mode of operation.
 * We run all outstandingCommands, until either we run out of things
 * to process, or we update the dom nodes of the same surface twice.
 * Then we wait for all textures to load, and then we request am
 * animation frame and apply the display changes. Then we loop back and
 * handle outstanding commands again.
 *
 * The reason for stopping if we update the same tree twice is that
 * the delta operations generally assume that the previous dom tree
 * is in pristine condition.
 */
function handleOutstanding()
{
    var display_commands = new Array();
    var new_textures = new Array();
    var modified_trees = {};

    if (outstandingDisplayCommands != null)
        return;

    while (outstandingCommands.length > 0) {
        var cmd = outstandingCommands.shift();
        if (!handleCommands(cmd, display_commands, new_textures, modified_trees)) {
            outstandingCommands.unshift(cmd);
            break;
        }
    }

    if (display_commands.length > 0)
        outstandingDisplayCommands = display_commands;

    if (new_textures.length > 0) {
        var decodes = [];
        for (var i = 0; i < new_textures.length; i++) {
            decodes.push(new_textures[i].decoded);
        }
        Promise.allSettled(decodes).then(
            () => {
                handleOutstandingDisplayCommands();
            });
    } else {
        handleOutstandingDisplayCommands();
    }

}

function BinCommands(message) {
    this.arraybuffer = message;
    this.dataview = new DataView(message);
    this.length = this.arraybuffer.byteLength;
    this.pos = 0;
}

BinCommands.prototype.get_uint8 = function() {
    return this.dataview.getUint8(this.pos++);
};
BinCommands.prototype.get_bool = function() {
    return this.dataview.getUint8(this.pos++) != 0;
};
BinCommands.prototype.get_flags = function() {
    return this.dataview.getUint8(this.pos++);
}
BinCommands.prototype.get_16 = function() {
    var v = this.dataview.getUint16(this.pos, true);
    this.pos = this.pos + 2;
    return v;
};
BinCommands.prototype.get_16s = function() {
    var v = this.dataview.getInt16(this.pos, true);
    this.pos = this.pos + 2;
    return v;
};
BinCommands.prototype.get_32 = function() {
    var v = this.dataview.getUint32(this.pos, true);
    this.pos = this.pos + 4;
    return v;
};
BinCommands.prototype.get_nodes = function() {
    var len = this.get_32();
    var node_data = new DataView(this.arraybuffer, this.pos, len * 4);
    this.pos = this.pos + len * 4;
    return node_data;
};
BinCommands.prototype.get_data = function() {
    var size = this.get_32();
    var data = new Uint8Array (this.arraybuffer, this.pos, size);
    this.pos = this.pos + size;
    return data;
};

var active = false;
function handleMessage(message)
{
    if (sessionInvalidated)
        return;

    lastPongTime = Date.now();     /* any inbound data proves the link is alive */

    if (!active) {
        start();
        active = true;
    }

    var cmd = new BinCommands(message);
    outstandingCommands.push(cmd);
    if (outstandingCommands.length == 1) {
        handleOutstanding();
    }
}

/* Apply a surface's input region (BROADWAY_OP_SET_INPUT_REGION):
 *  mode 0 (whole): the whole surface div takes pointer events (the default).
 *  mode 1 (empty): nothing does - click-through (e.g. GtkTextHandle).
 *  mode 2 (rect):  only the given rect is interactive; everything outside it
 *                  (e.g. a popover's shadow margin) passes clicks through.
 * For the rect case the surface div goes pointer-events:none - its rendered
 * children inherit that - and one transparent hit div over the rect captures
 * input. A click on it still resolves to this surface via getSurfaceId (the hit
 * div's parent is surface.div, which carries .surface). */
function setInputRegion(surface, mode, x, y, w, h) {
    var div = surface.div;
    if (surface.inputHit) {
        surface.inputHit.remove();
        surface.inputHit = null;
    }
    if (mode == 2) {
        div.style.pointerEvents = "none";
        var hit = document.createElement("div");
        hit.style.cssText = "position:absolute;pointer-events:auto;background:transparent;";
        hit.style.left = x + "px";
        hit.style.top = y + "px";
        hit.style.width = w + "px";
        hit.style.height = h + "px";
        div.appendChild(hit);
        surface.inputHit = hit;
    } else {
        div.style.pointerEvents = (mode == 1) ? "none" : "auto";
    }
}

function getSurfaceId(ev) {
    var target = ev.target;
    while (target && target.surface == undefined) {
        if (target == document)
            return 0;
        target = target.parentNode;
    }

    return (target && target.surface) ? target.surface.id : 0;
}

/* GTK only needs the latest pointer position, so buffer moves and send one per
 * frame; a motion flood can't then fill the websocket and delay a later click
 * or key (head-of-line blocking). Discrete events flush the pending move first
 * to preserve ordering. */
var pendingMove = null;
var pendingMoveScheduled = false;

function flushPendingMove()
{
    if (pendingMove == null)
        return;
    var args = pendingMove;
    pendingMove = null;
    rawSendInput(BROADWAY_EVENT_POINTER_MOVE, args);
}

function pendingMoveFrame()
{
    pendingMoveScheduled = false;
    flushPendingMove();
}

function queuePointerMove(args)
{
    pendingMove = args;
    if (!pendingMoveScheduled) {
        pendingMoveScheduled = true;
        window.requestAnimationFrame(pendingMoveFrame);
    }
}

function sendInput(cmd, args)
{
    /* Flush any buffered move before a discrete event to keep ordering. */
    if (cmd == BROADWAY_EVENT_POINTER_MOVE) {
        queuePointerMove(args);
        return;
    }
    flushPendingMove();
    rawSendInput(cmd, args);
}

function rawSendInput(cmd, args)
{
    if (inputSocket == null)
        return;

    /* Pack fields straight into the buffer; avoids the per-event concat()/
     * forEach() closure on the move/wheel hot path. */
    var buffer = new ArrayBuffer((args.length + 3) * 4);
    var view = new DataView(buffer);
    view.setInt32(0, cmd, false);
    view.setInt32(4, lastSerial, false);
    view.setInt32(8, lastTimeStamp, false);
    for (var i = 0; i < args.length; i++)
        view.setInt32((i + 3) * 4, args[i], false);

    inputSocket.send(buffer);
}

/* Best-effort copy into the host clipboard via the hidden textarea. Works on
 * insecure (http) origins where navigator.clipboard is unavailable, but the
 * browser may ignore execCommand('copy') outside a user gesture. */
function copyViaTextarea(text)
{
    if (!clipboardArea) {
        console.warn("broadway: cannot copy to host clipboard (no clipboard API)");
        return;
    }

    var prev = clipboardArea.value;
    clipboardArea.value = text;
    clipboardArea.select();

    var ok = false;
    try { ok = document.execCommand("copy"); } catch (e) { ok = false; }

    clipboardArea.value = prev;
    /* select() stole focus from the OSK input; restore its intended state. */
    if (clipboardArea === fakeInput) {
        if (showKeyboard)
            fakeInput.focus();
        else
            fakeInput.blur();
    }
    if (!ok)
        console.warn("broadway: copy to host clipboard was rejected by the browser");
}

function setHostClipboard(text)
{
    if (navigator.clipboard && navigator.clipboard.writeText) {
        navigator.clipboard.writeText(text).catch(function (e) {
            copyViaTextarea(text);
        });
    } else {
        copyViaTextarea(text);
    }
}

function sendClipboardContents(text, id)
{
    if (inputSocket == null)
        return;

    /* Echo back the request id so the daemon can route this answer to the one
     * client (and read) that asked for it. Layout: type, serial, time, id, len,
     * then the UTF-8 text. */
    var bytes = new TextEncoder().encode(text == null ? "" : text);
    var buffer = new ArrayBuffer(4 * 5 + bytes.length);
    var view = new DataView(buffer);
    view.setInt32(0, BROADWAY_EVENT_CLIPBOARD_CONTENTS, false);
    view.setInt32(4, lastSerial, false);
    view.setInt32(8, lastTimeStamp, false);
    view.setInt32(12, id, false);
    view.setInt32(16, bytes.length, false);
    new Uint8Array(buffer, 20).set(bytes);

    inputSocket.send(buffer);
}

/* Commit literal text into the focused GTK widget as synthetic key events. A
 * keyval of (codepoint | 0x01000000) is GDK's direct Unicode encoding. */
function commitTextToGtk(text)
{
    if (!text)
        return;

    var state = lastState & ~(GDK_SHIFT_MASK|GDK_CONTROL_MASK|GDK_ALT_MASK);
    text = text.replace(/\r\n?/g, "\n");

    /* Iterate by code point so astral chars (e.g. emoji) commit as one keyval. */
    for (var ch of text) {
        var cp = ch.codePointAt(0);
        /* Control chars need their named GDK keysym, not a direct-Unicode one:
         * GtkText/GtkTextView treat 0xFF0D/0xFF09 as Return/Tab, but ignore a
         * direct-Unicode LF, so pasted/IME-committed newlines would be lost. */
        var keysym;
        if (cp == 0x0A)      /* newline */
            keysym = 0xFF0D; /* GDK_KEY_Return */
        else if (cp == 0x09) /* tab */
            keysym = 0xFF09; /* GDK_KEY_Tab */
        else
            keysym = cp | 0x01000000;
        sendInput(BROADWAY_EVENT_KEY_PRESS, [keysym, state]);
        sendInput(BROADWAY_EVENT_KEY_RELEASE, [keysym, state]);
    }
}

/* Touch paste: forward straight into GTK rather than via pasteCache, which only
 * feeds GTK's own clipboard request (desktop Ctrl+V) and would double-insert. */
function capturePaste(text)
{
    lastPasteForwardTime = Date.now();
    commitTextToGtk(text);
}

function getPositionsFromAbsCoord(absX, absY, relativeId) {
    var res = Object();

    res.rootX = absX;
    res.rootY = absY;
    res.winX = absX;
    res.winY = absY;
    if (relativeId != 0) {
        var surface = surfaces[relativeId];
        res.winX = res.winX - surface.x;
        res.winY = res.winY - surface.y;
    }

    return res;
}

function getPositionsFromEvent(ev, relativeId) {
    var absX, absY;
    /* Page coords are physical CSS pixels; the #zoomRoot wrapper is scaled by
     * zoomFactor (origin 0 0), so surface root coords are page coords / zoom.
     * At zoom 1 this is a no-op (the desktop path is unchanged). */
    absX = ev.pageX / zoomFactor;
    absY = ev.pageY / zoomFactor;
    var res = getPositionsFromAbsCoord(absX, absY, relativeId);

    lastX = res.rootX;
    lastY = res.rootY;

    return res;
}

function getEffectiveEventTarget (id) {
    if (grab.surface != null) {
        if (!grab.ownerEvents)
            return grab.surface;
        if (id == 0)
            return grab.surface;
    }
    return id;
}

function applyKeyboard() {
    if (fakeInput == null)
        return;
    if (showKeyboard) {
        if (isAndroidChrome) {
            fakeInput.blur();
            fakeInput.value = ' '.repeat(80); // TODO: Should be exchange with broadway server
                                              // to bring real value here.
        }
        fakeInput.focus();
    }
    else
        fakeInput.blur();
}

/* Fallback path: re-assert the keyboard state on a touch (a real gesture), in
 * case applyKeyboard() ran while the transient activation window was closed. */
function updateKeyboardStatus() {
    if (fakeInput != null && showKeyboardChanged) {
        showKeyboardChanged = false;
        applyKeyboard();
    }
}

function updateForEvent(ev) {
    lastState &= ~(GDK_SHIFT_MASK|GDK_CONTROL_MASK|GDK_ALT_MASK);
    if (ev.shiftKey)
        lastState |= GDK_SHIFT_MASK;
    if (ev.ctrlKey)
        lastState |= GDK_CONTROL_MASK;
    if (ev.altKey)
        lastState |= GDK_ALT_MASK;

    lastTimeStamp = ev.timeStamp;
}

function onMouseMove (ev) {
    updateForEvent(ev);
    var id = getSurfaceId(ev);
    id = getEffectiveEventTarget (id);
    var pos = getPositionsFromEvent(ev, id);
    sendInput (BROADWAY_EVENT_POINTER_MOVE, [realSurfaceWithMouse, id, pos.rootX, pos.rootY, pos.winX, pos.winY, lastState]);
}

function onMouseOver (ev) {
    updateForEvent(ev);

    /* During an implicit pointer grab (a mouse button held after pressing a
     * widget) the pointer is confined to the grabbed surface, like an X11/GDK
     * implicit grab. Suppress surface-crossing enter/leave -- which the document
     * handlers otherwise fire on every DOM node boundary, even within one
     * surface -- so e.g. dragging a scrollbar keeps tracking when the cursor
     * wanders off the widget. Explicit grabs (menus/popovers) are unaffected. */
    if (grab.surface != null && grab.implicit)
        return;

    var id = getSurfaceId(ev);
    realSurfaceWithMouse = id;
    id = getEffectiveEventTarget (id);
    var pos = getPositionsFromEvent(ev, id);
    surfaceWithMouse = id;
    if (surfaceWithMouse != 0) {
        sendInput (BROADWAY_EVENT_ENTER, [realSurfaceWithMouse, id, pos.rootX, pos.rootY, pos.winX, pos.winY, lastState, GDK_CROSSING_NORMAL]);
    }
}

function onMouseOut (ev) {
    updateForEvent(ev);

    /* See onMouseOver: keep the implicit-grab surface even when the cursor
     * leaves the widget, so a button drag isn't interrupted. */
    if (grab.surface != null && grab.implicit)
        return;

    var id = getSurfaceId(ev);
    var origId = id;
    id = getEffectiveEventTarget (id);
    var pos = getPositionsFromEvent(ev, id);

    if (id != 0) {
        sendInput (BROADWAY_EVENT_LEAVE, [realSurfaceWithMouse, id, pos.rootX, pos.rootY, pos.winX, pos.winY, lastState, GDK_CROSSING_NORMAL]);
    }
    realSurfaceWithMouse = 0;
    surfaceWithMouse = 0;
}

function doGrab(id, ownerEvents, implicit) {
    var pos;

    if (surfaceWithMouse != id) {
        if (surfaceWithMouse != 0) {
            pos = getPositionsFromAbsCoord(lastX, lastY, surfaceWithMouse);
            sendInput (BROADWAY_EVENT_LEAVE, [realSurfaceWithMouse, surfaceWithMouse, pos.rootX, pos.rootY, pos.winX, pos.winY, lastState, GDK_CROSSING_GRAB]);
        }
        pos = getPositionsFromAbsCoord(lastX, lastY, id);
        sendInput (BROADWAY_EVENT_ENTER, [realSurfaceWithMouse, id, pos.rootX, pos.rootY, pos.winX, pos.winY, lastState, GDK_CROSSING_GRAB]);
        surfaceWithMouse = id;
    }

    grab.surface = id;
    grab.ownerEvents = ownerEvents;
    grab.implicit = implicit;
}

function doUngrab() {
    var pos;
    if (realSurfaceWithMouse != surfaceWithMouse) {
        if (surfaceWithMouse != 0) {
            pos = getPositionsFromAbsCoord(lastX, lastY, surfaceWithMouse);
            sendInput (BROADWAY_EVENT_LEAVE, [realSurfaceWithMouse, surfaceWithMouse, pos.rootX, pos.rootY, pos.winX, pos.winY, lastState, GDK_CROSSING_UNGRAB]);
        }
        if (realSurfaceWithMouse != 0) {
            pos = getPositionsFromAbsCoord(lastX, lastY, realSurfaceWithMouse);
            sendInput (BROADWAY_EVENT_ENTER, [realSurfaceWithMouse, realSurfaceWithMouse, pos.rootX, pos.rootY, pos.winX, pos.winY, lastState, GDK_CROSSING_UNGRAB]);
        }
        surfaceWithMouse = realSurfaceWithMouse;
    }
    grab.surface = null;
}

function onMouseDown (ev) {
    updateForEvent(ev);
    var button = ev.button + 1;
    lastState = lastState | getButtonMask (button);
    var id = getSurfaceId(ev);
    id = getEffectiveEventTarget (id);

    var pos = getPositionsFromEvent(ev, id);
    if (grab.surface == null)
        doGrab (id, false, true);
    sendInput (BROADWAY_EVENT_BUTTON_PRESS, [realSurfaceWithMouse, id, pos.rootX, pos.rootY, pos.winX, pos.winY, lastState, button]);
    return false;
}

function onMouseUp (ev) {
    updateForEvent(ev);
    var button = ev.button + 1;
    lastState = lastState & ~getButtonMask (button);
    var evId = getSurfaceId(ev);
    id = getEffectiveEventTarget (evId);
    var pos = getPositionsFromEvent(ev, id);

    sendInput (BROADWAY_EVENT_BUTTON_RELEASE, [realSurfaceWithMouse, id, pos.rootX, pos.rootY, pos.winX, pos.winY, lastState, button]);

    if (grab.surface != null && grab.implicit)
        doUngrab();

    return false;
}

/* Some of the keyboard handling code is from noVNC and
 * (c) Joel Martin (github@martintribe.org), used with permission
 *  Original code at:
 * https://github.com/kanaka/noVNC/blob/master/include/input.js
 */

var unicodeTable = {
    0x0104: 0x01a1,
    0x02D8: 0x01a2,
    0x0141: 0x01a3,
    0x013D: 0x01a5,
    0x015A: 0x01a6,
    0x0160: 0x01a9,
    0x015E: 0x01aa,
    0x0164: 0x01ab,
    0x0179: 0x01ac,
    0x017D: 0x01ae,
    0x017B: 0x01af,
    0x0105: 0x01b1,
    0x02DB: 0x01b2,
    0x0142: 0x01b3,
    0x013E: 0x01b5,
    0x015B: 0x01b6,
    0x02C7: 0x01b7,
    0x0161: 0x01b9,
    0x015F: 0x01ba,
    0x0165: 0x01bb,
    0x017A: 0x01bc,
    0x02DD: 0x01bd,
    0x017E: 0x01be,
    0x017C: 0x01bf,
    0x0154: 0x01c0,
    0x0102: 0x01c3,
    0x0139: 0x01c5,
    0x0106: 0x01c6,
    0x010C: 0x01c8,
    0x0118: 0x01ca,
    0x011A: 0x01cc,
    0x010E: 0x01cf,
    0x0110: 0x01d0,
    0x0143: 0x01d1,
    0x0147: 0x01d2,
    0x0150: 0x01d5,
    0x0158: 0x01d8,
    0x016E: 0x01d9,
    0x0170: 0x01db,
    0x0162: 0x01de,
    0x0155: 0x01e0,
    0x0103: 0x01e3,
    0x013A: 0x01e5,
    0x0107: 0x01e6,
    0x010D: 0x01e8,
    0x0119: 0x01ea,
    0x011B: 0x01ec,
    0x010F: 0x01ef,
    0x0111: 0x01f0,
    0x0144: 0x01f1,
    0x0148: 0x01f2,
    0x0151: 0x01f5,
    0x0171: 0x01fb,
    0x0159: 0x01f8,
    0x016F: 0x01f9,
    0x0163: 0x01fe,
    0x02D9: 0x01ff,
    0x0126: 0x02a1,
    0x0124: 0x02a6,
    0x0130: 0x02a9,
    0x011E: 0x02ab,
    0x0134: 0x02ac,
    0x0127: 0x02b1,
    0x0125: 0x02b6,
    0x0131: 0x02b9,
    0x011F: 0x02bb,
    0x0135: 0x02bc,
    0x010A: 0x02c5,
    0x0108: 0x02c6,
    0x0120: 0x02d5,
    0x011C: 0x02d8,
    0x016C: 0x02dd,
    0x015C: 0x02de,
    0x010B: 0x02e5,
    0x0109: 0x02e6,
    0x0121: 0x02f5,
    0x011D: 0x02f8,
    0x016D: 0x02fd,
    0x015D: 0x02fe,
    0x0138: 0x03a2,
    0x0156: 0x03a3,
    0x0128: 0x03a5,
    0x013B: 0x03a6,
    0x0112: 0x03aa,
    0x0122: 0x03ab,
    0x0166: 0x03ac,
    0x0157: 0x03b3,
    0x0129: 0x03b5,
    0x013C: 0x03b6,
    0x0113: 0x03ba,
    0x0123: 0x03bb,
    0x0167: 0x03bc,
    0x014A: 0x03bd,
    0x014B: 0x03bf,
    0x0100: 0x03c0,
    0x012E: 0x03c7,
    0x0116: 0x03cc,
    0x012A: 0x03cf,
    0x0145: 0x03d1,
    0x014C: 0x03d2,
    0x0136: 0x03d3,
    0x0172: 0x03d9,
    0x0168: 0x03dd,
    0x016A: 0x03de,
    0x0101: 0x03e0,
    0x012F: 0x03e7,
    0x0117: 0x03ec,
    0x012B: 0x03ef,
    0x0146: 0x03f1,
    0x014D: 0x03f2,
    0x0137: 0x03f3,
    0x0173: 0x03f9,
    0x0169: 0x03fd,
    0x016B: 0x03fe,
    0x1E02: 0x1001e02,
    0x1E03: 0x1001e03,
    0x1E0A: 0x1001e0a,
    0x1E80: 0x1001e80,
    0x1E82: 0x1001e82,
    0x1E0B: 0x1001e0b,
    0x1EF2: 0x1001ef2,
    0x1E1E: 0x1001e1e,
    0x1E1F: 0x1001e1f,
    0x1E40: 0x1001e40,
    0x1E41: 0x1001e41,
    0x1E56: 0x1001e56,
    0x1E81: 0x1001e81,
    0x1E57: 0x1001e57,
    0x1E83: 0x1001e83,
    0x1E60: 0x1001e60,
    0x1EF3: 0x1001ef3,
    0x1E84: 0x1001e84,
    0x1E85: 0x1001e85,
    0x1E61: 0x1001e61,
    0x0174: 0x1000174,
    0x1E6A: 0x1001e6a,
    0x0176: 0x1000176,
    0x0175: 0x1000175,
    0x1E6B: 0x1001e6b,
    0x0177: 0x1000177,
    0x0152: 0x13bc,
    0x0153: 0x13bd,
    0x0178: 0x13be,
    0x203E: 0x047e,
    0x3002: 0x04a1,
    0x300C: 0x04a2,
    0x300D: 0x04a3,
    0x3001: 0x04a4,
    0x30FB: 0x04a5,
    0x30F2: 0x04a6,
    0x30A1: 0x04a7,
    0x30A3: 0x04a8,
    0x30A5: 0x04a9,
    0x30A7: 0x04aa,
    0x30A9: 0x04ab,
    0x30E3: 0x04ac,
    0x30E5: 0x04ad,
    0x30E7: 0x04ae,
    0x30C3: 0x04af,
    0x30FC: 0x04b0,
    0x30A2: 0x04b1,
    0x30A4: 0x04b2,
    0x30A6: 0x04b3,
    0x30A8: 0x04b4,
    0x30AA: 0x04b5,
    0x30AB: 0x04b6,
    0x30AD: 0x04b7,
    0x30AF: 0x04b8,
    0x30B1: 0x04b9,
    0x30B3: 0x04ba,
    0x30B5: 0x04bb,
    0x30B7: 0x04bc,
    0x30B9: 0x04bd,
    0x30BB: 0x04be,
    0x30BD: 0x04bf,
    0x30BF: 0x04c0,
    0x30C1: 0x04c1,
    0x30C4: 0x04c2,
    0x30C6: 0x04c3,
    0x30C8: 0x04c4,
    0x30CA: 0x04c5,
    0x30CB: 0x04c6,
    0x30CC: 0x04c7,
    0x30CD: 0x04c8,
    0x30CE: 0x04c9,
    0x30CF: 0x04ca,
    0x30D2: 0x04cb,
    0x30D5: 0x04cc,
    0x30D8: 0x04cd,
    0x30DB: 0x04ce,
    0x30DE: 0x04cf,
    0x30DF: 0x04d0,
    0x30E0: 0x04d1,
    0x30E1: 0x04d2,
    0x30E2: 0x04d3,
    0x30E4: 0x04d4,
    0x30E6: 0x04d5,
    0x30E8: 0x04d6,
    0x30E9: 0x04d7,
    0x30EA: 0x04d8,
    0x30EB: 0x04d9,
    0x30EC: 0x04da,
    0x30ED: 0x04db,
    0x30EF: 0x04dc,
    0x30F3: 0x04dd,
    0x309B: 0x04de,
    0x309C: 0x04df,
    0x06F0: 0x10006f0,
    0x06F1: 0x10006f1,
    0x06F2: 0x10006f2,
    0x06F3: 0x10006f3,
    0x06F4: 0x10006f4,
    0x06F5: 0x10006f5,
    0x06F6: 0x10006f6,
    0x06F7: 0x10006f7,
    0x06F8: 0x10006f8,
    0x06F9: 0x10006f9,
    0x066A: 0x100066a,
    0x0670: 0x1000670,
    0x0679: 0x1000679,
    0x067E: 0x100067e,
    0x0686: 0x1000686,
    0x0688: 0x1000688,
    0x0691: 0x1000691,
    0x060C: 0x05ac,
    0x06D4: 0x10006d4,
    0x0660: 0x1000660,
    0x0661: 0x1000661,
    0x0662: 0x1000662,
    0x0663: 0x1000663,
    0x0664: 0x1000664,
    0x0665: 0x1000665,
    0x0666: 0x1000666,
    0x0667: 0x1000667,
    0x0668: 0x1000668,
    0x0669: 0x1000669,
    0x061B: 0x05bb,
    0x061F: 0x05bf,
    0x0621: 0x05c1,
    0x0622: 0x05c2,
    0x0623: 0x05c3,
    0x0624: 0x05c4,
    0x0625: 0x05c5,
    0x0626: 0x05c6,
    0x0627: 0x05c7,
    0x0628: 0x05c8,
    0x0629: 0x05c9,
    0x062A: 0x05ca,
    0x062B: 0x05cb,
    0x062C: 0x05cc,
    0x062D: 0x05cd,
    0x062E: 0x05ce,
    0x062F: 0x05cf,
    0x0630: 0x05d0,
    0x0631: 0x05d1,
    0x0632: 0x05d2,
    0x0633: 0x05d3,
    0x0634: 0x05d4,
    0x0635: 0x05d5,
    0x0636: 0x05d6,
    0x0637: 0x05d7,
    0x0638: 0x05d8,
    0x0639: 0x05d9,
    0x063A: 0x05da,
    0x0640: 0x05e0,
    0x0641: 0x05e1,
    0x0642: 0x05e2,
    0x0643: 0x05e3,
    0x0644: 0x05e4,
    0x0645: 0x05e5,
    0x0646: 0x05e6,
    0x0647: 0x05e7,
    0x0648: 0x05e8,
    0x0649: 0x05e9,
    0x064A: 0x05ea,
    0x064B: 0x05eb,
    0x064C: 0x05ec,
    0x064D: 0x05ed,
    0x064E: 0x05ee,
    0x064F: 0x05ef,
    0x0650: 0x05f0,
    0x0651: 0x05f1,
    0x0652: 0x05f2,
    0x0653: 0x1000653,
    0x0654: 0x1000654,
    0x0655: 0x1000655,
    0x0698: 0x1000698,
    0x06A4: 0x10006a4,
    0x06A9: 0x10006a9,
    0x06AF: 0x10006af,
    0x06BA: 0x10006ba,
    0x06BE: 0x10006be,
    0x06CC: 0x10006cc,
    0x06D2: 0x10006d2,
    0x06C1: 0x10006c1,
    0x0492: 0x1000492,
    0x0493: 0x1000493,
    0x0496: 0x1000496,
    0x0497: 0x1000497,
    0x049A: 0x100049a,
    0x049B: 0x100049b,
    0x049C: 0x100049c,
    0x049D: 0x100049d,
    0x04A2: 0x10004a2,
    0x04A3: 0x10004a3,
    0x04AE: 0x10004ae,
    0x04AF: 0x10004af,
    0x04B0: 0x10004b0,
    0x04B1: 0x10004b1,
    0x04B2: 0x10004b2,
    0x04B3: 0x10004b3,
    0x04B6: 0x10004b6,
    0x04B7: 0x10004b7,
    0x04B8: 0x10004b8,
    0x04B9: 0x10004b9,
    0x04BA: 0x10004ba,
    0x04BB: 0x10004bb,
    0x04D8: 0x10004d8,
    0x04D9: 0x10004d9,
    0x04E2: 0x10004e2,
    0x04E3: 0x10004e3,
    0x04E8: 0x10004e8,
    0x04E9: 0x10004e9,
    0x04EE: 0x10004ee,
    0x04EF: 0x10004ef,
    0x0452: 0x06a1,
    0x0453: 0x06a2,
    0x0451: 0x06a3,
    0x0454: 0x06a4,
    0x0455: 0x06a5,
    0x0456: 0x06a6,
    0x0457: 0x06a7,
    0x0458: 0x06a8,
    0x0459: 0x06a9,
    0x045A: 0x06aa,
    0x045B: 0x06ab,
    0x045C: 0x06ac,
    0x0491: 0x06ad,
    0x045E: 0x06ae,
    0x045F: 0x06af,
    0x2116: 0x06b0,
    0x0402: 0x06b1,
    0x0403: 0x06b2,
    0x0401: 0x06b3,
    0x0404: 0x06b4,
    0x0405: 0x06b5,
    0x0406: 0x06b6,
    0x0407: 0x06b7,
    0x0408: 0x06b8,
    0x0409: 0x06b9,
    0x040A: 0x06ba,
    0x040B: 0x06bb,
    0x040C: 0x06bc,
    0x0490: 0x06bd,
    0x040E: 0x06be,
    0x040F: 0x06bf,
    0x044E: 0x06c0,
    0x0430: 0x06c1,
    0x0431: 0x06c2,
    0x0446: 0x06c3,
    0x0434: 0x06c4,
    0x0435: 0x06c5,
    0x0444: 0x06c6,
    0x0433: 0x06c7,
    0x0445: 0x06c8,
    0x0438: 0x06c9,
    0x0439: 0x06ca,
    0x043A: 0x06cb,
    0x043B: 0x06cc,
    0x043C: 0x06cd,
    0x043D: 0x06ce,
    0x043E: 0x06cf,
    0x043F: 0x06d0,
    0x044F: 0x06d1,
    0x0440: 0x06d2,
    0x0441: 0x06d3,
    0x0442: 0x06d4,
    0x0443: 0x06d5,
    0x0436: 0x06d6,
    0x0432: 0x06d7,
    0x044C: 0x06d8,
    0x044B: 0x06d9,
    0x0437: 0x06da,
    0x0448: 0x06db,
    0x044D: 0x06dc,
    0x0449: 0x06dd,
    0x0447: 0x06de,
    0x044A: 0x06df,
    0x042E: 0x06e0,
    0x0410: 0x06e1,
    0x0411: 0x06e2,
    0x0426: 0x06e3,
    0x0414: 0x06e4,
    0x0415: 0x06e5,
    0x0424: 0x06e6,
    0x0413: 0x06e7,
    0x0425: 0x06e8,
    0x0418: 0x06e9,
    0x0419: 0x06ea,
    0x041A: 0x06eb,
    0x041B: 0x06ec,
    0x041C: 0x06ed,
    0x041D: 0x06ee,
    0x041E: 0x06ef,
    0x041F: 0x06f0,
    0x042F: 0x06f1,
    0x0420: 0x06f2,
    0x0421: 0x06f3,
    0x0422: 0x06f4,
    0x0423: 0x06f5,
    0x0416: 0x06f6,
    0x0412: 0x06f7,
    0x042C: 0x06f8,
    0x042B: 0x06f9,
    0x0417: 0x06fa,
    0x0428: 0x06fb,
    0x042D: 0x06fc,
    0x0429: 0x06fd,
    0x0427: 0x06fe,
    0x042A: 0x06ff,
    0x0386: 0x07a1,
    0x0388: 0x07a2,
    0x0389: 0x07a3,
    0x038A: 0x07a4,
    0x03AA: 0x07a5,
    0x038C: 0x07a7,
    0x038E: 0x07a8,
    0x03AB: 0x07a9,
    0x038F: 0x07ab,
    0x0385: 0x07ae,
    0x2015: 0x07af,
    0x03AC: 0x07b1,
    0x03AD: 0x07b2,
    0x03AE: 0x07b3,
    0x03AF: 0x07b4,
    0x03CA: 0x07b5,
    0x0390: 0x07b6,
    0x03CC: 0x07b7,
    0x03CD: 0x07b8,
    0x03CB: 0x07b9,
    0x03B0: 0x07ba,
    0x03CE: 0x07bb,
    0x0391: 0x07c1,
    0x0392: 0x07c2,
    0x0393: 0x07c3,
    0x0394: 0x07c4,
    0x0395: 0x07c5,
    0x0396: 0x07c6,
    0x0397: 0x07c7,
    0x0398: 0x07c8,
    0x0399: 0x07c9,
    0x039A: 0x07ca,
    0x039B: 0x07cb,
    0x039C: 0x07cc,
    0x039D: 0x07cd,
    0x039E: 0x07ce,
    0x039F: 0x07cf,
    0x03A0: 0x07d0,
    0x03A1: 0x07d1,
    0x03A3: 0x07d2,
    0x03A4: 0x07d4,
    0x03A5: 0x07d5,
    0x03A6: 0x07d6,
    0x03A7: 0x07d7,
    0x03A8: 0x07d8,
    0x03A9: 0x07d9,
    0x03B1: 0x07e1,
    0x03B2: 0x07e2,
    0x03B3: 0x07e3,
    0x03B4: 0x07e4,
    0x03B5: 0x07e5,
    0x03B6: 0x07e6,
    0x03B7: 0x07e7,
    0x03B8: 0x07e8,
    0x03B9: 0x07e9,
    0x03BA: 0x07ea,
    0x03BB: 0x07eb,
    0x03BC: 0x07ec,
    0x03BD: 0x07ed,
    0x03BE: 0x07ee,
    0x03BF: 0x07ef,
    0x03C0: 0x07f0,
    0x03C1: 0x07f1,
    0x03C3: 0x07f2,
    0x03C2: 0x07f3,
    0x03C4: 0x07f4,
    0x03C5: 0x07f5,
    0x03C6: 0x07f6,
    0x03C7: 0x07f7,
    0x03C8: 0x07f8,
    0x03C9: 0x07f9,
    0x23B7: 0x08a1,
    0x2320: 0x08a4,
    0x2321: 0x08a5,
    0x23A1: 0x08a7,
    0x23A3: 0x08a8,
    0x23A4: 0x08a9,
    0x23A6: 0x08aa,
    0x239B: 0x08ab,
    0x239D: 0x08ac,
    0x239E: 0x08ad,
    0x23A0: 0x08ae,
    0x23A8: 0x08af,
    0x23AC: 0x08b0,
    0x2264: 0x08bc,
    0x2260: 0x08bd,
    0x2265: 0x08be,
    0x222B: 0x08bf,
    0x2234: 0x08c0,
    0x221D: 0x08c1,
    0x221E: 0x08c2,
    0x2207: 0x08c5,
    0x223C: 0x08c8,
    0x2243: 0x08c9,
    0x21D4: 0x08cd,
    0x21D2: 0x08ce,
    0x2261: 0x08cf,
    0x221A: 0x08d6,
    0x2282: 0x08da,
    0x2283: 0x08db,
    0x2229: 0x08dc,
    0x222A: 0x08dd,
    0x2227: 0x08de,
    0x2228: 0x08df,
    0x2202: 0x08ef,
    0x0192: 0x08f6,
    0x2190: 0x08fb,
    0x2191: 0x08fc,
    0x2192: 0x08fd,
    0x2193: 0x08fe,
    0x25C6: 0x09e0,
    0x2592: 0x09e1,
    0x2409: 0x09e2,
    0x240C: 0x09e3,
    0x240D: 0x09e4,
    0x240A: 0x09e5,
    0x2424: 0x09e8,
    0x240B: 0x09e9,
    0x2518: 0x09ea,
    0x2510: 0x09eb,
    0x250C: 0x09ec,
    0x2514: 0x09ed,
    0x253C: 0x09ee,
    0x23BA: 0x09ef,
    0x23BB: 0x09f0,
    0x2500: 0x09f1,
    0x23BC: 0x09f2,
    0x23BD: 0x09f3,
    0x251C: 0x09f4,
    0x2524: 0x09f5,
    0x2534: 0x09f6,
    0x252C: 0x09f7,
    0x2502: 0x09f8,
    0x2003: 0x0aa1,
    0x2002: 0x0aa2,
    0x2004: 0x0aa3,
    0x2005: 0x0aa4,
    0x2007: 0x0aa5,
    0x2008: 0x0aa6,
    0x2009: 0x0aa7,
    0x200A: 0x0aa8,
    0x2014: 0x0aa9,
    0x2013: 0x0aaa,
    0x2026: 0x0aae,
    0x2025: 0x0aaf,
    0x2153: 0x0ab0,
    0x2154: 0x0ab1,
    0x2155: 0x0ab2,
    0x2156: 0x0ab3,
    0x2157: 0x0ab4,
    0x2158: 0x0ab5,
    0x2159: 0x0ab6,
    0x215A: 0x0ab7,
    0x2105: 0x0ab8,
    0x2012: 0x0abb,
    0x215B: 0x0ac3,
    0x215C: 0x0ac4,
    0x215D: 0x0ac5,
    0x215E: 0x0ac6,
    0x2122: 0x0ac9,
    0x2018: 0x0ad0,
    0x2019: 0x0ad1,
    0x201C: 0x0ad2,
    0x201D: 0x0ad3,
    0x211E: 0x0ad4,
    0x2032: 0x0ad6,
    0x2033: 0x0ad7,
    0x271D: 0x0ad9,
    0x2663: 0x0aec,
    0x2666: 0x0aed,
    0x2665: 0x0aee,
    0x2720: 0x0af0,
    0x2020: 0x0af1,
    0x2021: 0x0af2,
    0x2713: 0x0af3,
    0x2717: 0x0af4,
    0x266F: 0x0af5,
    0x266D: 0x0af6,
    0x2642: 0x0af7,
    0x2640: 0x0af8,
    0x260E: 0x0af9,
    0x2315: 0x0afa,
    0x2117: 0x0afb,
    0x2038: 0x0afc,
    0x201A: 0x0afd,
    0x201E: 0x0afe,
    0x22A4: 0x0bc2,
    0x230A: 0x0bc4,
    0x2218: 0x0bca,
    0x2395: 0x0bcc,
    0x22A5: 0x0bce,
    0x25CB: 0x0bcf,
    0x2308: 0x0bd3,
    0x22A3: 0x0bdc,
    0x22A2: 0x0bfc,
    0x2017: 0x0cdf,
    0x05D0: 0x0ce0,
    0x05D1: 0x0ce1,
    0x05D2: 0x0ce2,
    0x05D3: 0x0ce3,
    0x05D4: 0x0ce4,
    0x05D5: 0x0ce5,
    0x05D6: 0x0ce6,
    0x05D7: 0x0ce7,
    0x05D8: 0x0ce8,
    0x05D9: 0x0ce9,
    0x05DA: 0x0cea,
    0x05DB: 0x0ceb,
    0x05DC: 0x0cec,
    0x05DD: 0x0ced,
    0x05DE: 0x0cee,
    0x05DF: 0x0cef,
    0x05E0: 0x0cf0,
    0x05E1: 0x0cf1,
    0x05E2: 0x0cf2,
    0x05E3: 0x0cf3,
    0x05E4: 0x0cf4,
    0x05E5: 0x0cf5,
    0x05E6: 0x0cf6,
    0x05E7: 0x0cf7,
    0x05E8: 0x0cf8,
    0x05E9: 0x0cf9,
    0x05EA: 0x0cfa,
    0x0E01: 0x0da1,
    0x0E02: 0x0da2,
    0x0E03: 0x0da3,
    0x0E04: 0x0da4,
    0x0E05: 0x0da5,
    0x0E06: 0x0da6,
    0x0E07: 0x0da7,
    0x0E08: 0x0da8,
    0x0E09: 0x0da9,
    0x0E0A: 0x0daa,
    0x0E0B: 0x0dab,
    0x0E0C: 0x0dac,
    0x0E0D: 0x0dad,
    0x0E0E: 0x0dae,
    0x0E0F: 0x0daf,
    0x0E10: 0x0db0,
    0x0E11: 0x0db1,
    0x0E12: 0x0db2,
    0x0E13: 0x0db3,
    0x0E14: 0x0db4,
    0x0E15: 0x0db5,
    0x0E16: 0x0db6,
    0x0E17: 0x0db7,
    0x0E18: 0x0db8,
    0x0E19: 0x0db9,
    0x0E1A: 0x0dba,
    0x0E1B: 0x0dbb,
    0x0E1C: 0x0dbc,
    0x0E1D: 0x0dbd,
    0x0E1E: 0x0dbe,
    0x0E1F: 0x0dbf,
    0x0E20: 0x0dc0,
    0x0E21: 0x0dc1,
    0x0E22: 0x0dc2,
    0x0E23: 0x0dc3,
    0x0E24: 0x0dc4,
    0x0E25: 0x0dc5,
    0x0E26: 0x0dc6,
    0x0E27: 0x0dc7,
    0x0E28: 0x0dc8,
    0x0E29: 0x0dc9,
    0x0E2A: 0x0dca,
    0x0E2B: 0x0dcb,
    0x0E2C: 0x0dcc,
    0x0E2D: 0x0dcd,
    0x0E2E: 0x0dce,
    0x0E2F: 0x0dcf,
    0x0E30: 0x0dd0,
    0x0E31: 0x0dd1,
    0x0E32: 0x0dd2,
    0x0E33: 0x0dd3,
    0x0E34: 0x0dd4,
    0x0E35: 0x0dd5,
    0x0E36: 0x0dd6,
    0x0E37: 0x0dd7,
    0x0E38: 0x0dd8,
    0x0E39: 0x0dd9,
    0x0E3A: 0x0dda,
    0x0E3F: 0x0ddf,
    0x0E40: 0x0de0,
    0x0E41: 0x0de1,
    0x0E42: 0x0de2,
    0x0E43: 0x0de3,
    0x0E44: 0x0de4,
    0x0E45: 0x0de5,
    0x0E46: 0x0de6,
    0x0E47: 0x0de7,
    0x0E48: 0x0de8,
    0x0E49: 0x0de9,
    0x0E4A: 0x0dea,
    0x0E4B: 0x0deb,
    0x0E4C: 0x0dec,
    0x0E4D: 0x0ded,
    0x0E50: 0x0df0,
    0x0E51: 0x0df1,
    0x0E52: 0x0df2,
    0x0E53: 0x0df3,
    0x0E54: 0x0df4,
    0x0E55: 0x0df5,
    0x0E56: 0x0df6,
    0x0E57: 0x0df7,
    0x0E58: 0x0df8,
    0x0E59: 0x0df9,
    0x0587: 0x1000587,
    0x0589: 0x1000589,
    0x055D: 0x100055d,
    0x058A: 0x100058a,
    0x055C: 0x100055c,
    0x055B: 0x100055b,
    0x055E: 0x100055e,
    0x0531: 0x1000531,
    0x0561: 0x1000561,
    0x0532: 0x1000532,
    0x0562: 0x1000562,
    0x0533: 0x1000533,
    0x0563: 0x1000563,
    0x0534: 0x1000534,
    0x0564: 0x1000564,
    0x0535: 0x1000535,
    0x0565: 0x1000565,
    0x0536: 0x1000536,
    0x0566: 0x1000566,
    0x0537: 0x1000537,
    0x0567: 0x1000567,
    0x0538: 0x1000538,
    0x0568: 0x1000568,
    0x0539: 0x1000539,
    0x0569: 0x1000569,
    0x053A: 0x100053a,
    0x056A: 0x100056a,
    0x053B: 0x100053b,
    0x056B: 0x100056b,
    0x053C: 0x100053c,
    0x056C: 0x100056c,
    0x053D: 0x100053d,
    0x056D: 0x100056d,
    0x053E: 0x100053e,
    0x056E: 0x100056e,
    0x053F: 0x100053f,
    0x056F: 0x100056f,
    0x0540: 0x1000540,
    0x0570: 0x1000570,
    0x0541: 0x1000541,
    0x0571: 0x1000571,
    0x0542: 0x1000542,
    0x0572: 0x1000572,
    0x0543: 0x1000543,
    0x0573: 0x1000573,
    0x0544: 0x1000544,
    0x0574: 0x1000574,
    0x0545: 0x1000545,
    0x0575: 0x1000575,
    0x0546: 0x1000546,
    0x0576: 0x1000576,
    0x0547: 0x1000547,
    0x0577: 0x1000577,
    0x0548: 0x1000548,
    0x0578: 0x1000578,
    0x0549: 0x1000549,
    0x0579: 0x1000579,
    0x054A: 0x100054a,
    0x057A: 0x100057a,
    0x054B: 0x100054b,
    0x057B: 0x100057b,
    0x054C: 0x100054c,
    0x057C: 0x100057c,
    0x054D: 0x100054d,
    0x057D: 0x100057d,
    0x054E: 0x100054e,
    0x057E: 0x100057e,
    0x054F: 0x100054f,
    0x057F: 0x100057f,
    0x0550: 0x1000550,
    0x0580: 0x1000580,
    0x0551: 0x1000551,
    0x0581: 0x1000581,
    0x0552: 0x1000552,
    0x0582: 0x1000582,
    0x0553: 0x1000553,
    0x0583: 0x1000583,
    0x0554: 0x1000554,
    0x0584: 0x1000584,
    0x0555: 0x1000555,
    0x0585: 0x1000585,
    0x0556: 0x1000556,
    0x0586: 0x1000586,
    0x055A: 0x100055a,
    0x10D0: 0x10010d0,
    0x10D1: 0x10010d1,
    0x10D2: 0x10010d2,
    0x10D3: 0x10010d3,
    0x10D4: 0x10010d4,
    0x10D5: 0x10010d5,
    0x10D6: 0x10010d6,
    0x10D7: 0x10010d7,
    0x10D8: 0x10010d8,
    0x10D9: 0x10010d9,
    0x10DA: 0x10010da,
    0x10DB: 0x10010db,
    0x10DC: 0x10010dc,
    0x10DD: 0x10010dd,
    0x10DE: 0x10010de,
    0x10DF: 0x10010df,
    0x10E0: 0x10010e0,
    0x10E1: 0x10010e1,
    0x10E2: 0x10010e2,
    0x10E3: 0x10010e3,
    0x10E4: 0x10010e4,
    0x10E5: 0x10010e5,
    0x10E6: 0x10010e6,
    0x10E7: 0x10010e7,
    0x10E8: 0x10010e8,
    0x10E9: 0x10010e9,
    0x10EA: 0x10010ea,
    0x10EB: 0x10010eb,
    0x10EC: 0x10010ec,
    0x10ED: 0x10010ed,
    0x10EE: 0x10010ee,
    0x10EF: 0x10010ef,
    0x10F0: 0x10010f0,
    0x10F1: 0x10010f1,
    0x10F2: 0x10010f2,
    0x10F3: 0x10010f3,
    0x10F4: 0x10010f4,
    0x10F5: 0x10010f5,
    0x10F6: 0x10010f6,
    0x1E8A: 0x1001e8a,
    0x012C: 0x100012c,
    0x01B5: 0x10001b5,
    0x01E6: 0x10001e6,
    0x01D2: 0x10001d1,
    0x019F: 0x100019f,
    0x1E8B: 0x1001e8b,
    0x012D: 0x100012d,
    0x01B6: 0x10001b6,
    0x01E7: 0x10001e7,
    0x01D2: 0x10001d2,
    0x0275: 0x1000275,
    0x018F: 0x100018f,
    0x0259: 0x1000259,
    0x1E36: 0x1001e36,
    0x1E37: 0x1001e37,
    0x1EA0: 0x1001ea0,
    0x1EA1: 0x1001ea1,
    0x1EA2: 0x1001ea2,
    0x1EA3: 0x1001ea3,
    0x1EA4: 0x1001ea4,
    0x1EA5: 0x1001ea5,
    0x1EA6: 0x1001ea6,
    0x1EA7: 0x1001ea7,
    0x1EA8: 0x1001ea8,
    0x1EA9: 0x1001ea9,
    0x1EAA: 0x1001eaa,
    0x1EAB: 0x1001eab,
    0x1EAC: 0x1001eac,
    0x1EAD: 0x1001ead,
    0x1EAE: 0x1001eae,
    0x1EAF: 0x1001eaf,
    0x1EB0: 0x1001eb0,
    0x1EB1: 0x1001eb1,
    0x1EB2: 0x1001eb2,
    0x1EB3: 0x1001eb3,
    0x1EB4: 0x1001eb4,
    0x1EB5: 0x1001eb5,
    0x1EB6: 0x1001eb6,
    0x1EB7: 0x1001eb7,
    0x1EB8: 0x1001eb8,
    0x1EB9: 0x1001eb9,
    0x1EBA: 0x1001eba,
    0x1EBB: 0x1001ebb,
    0x1EBC: 0x1001ebc,
    0x1EBD: 0x1001ebd,
    0x1EBE: 0x1001ebe,
    0x1EBF: 0x1001ebf,
    0x1EC0: 0x1001ec0,
    0x1EC1: 0x1001ec1,
    0x1EC2: 0x1001ec2,
    0x1EC3: 0x1001ec3,
    0x1EC4: 0x1001ec4,
    0x1EC5: 0x1001ec5,
    0x1EC6: 0x1001ec6,
    0x1EC7: 0x1001ec7,
    0x1EC8: 0x1001ec8,
    0x1EC9: 0x1001ec9,
    0x1ECA: 0x1001eca,
    0x1ECB: 0x1001ecb,
    0x1ECC: 0x1001ecc,
    0x1ECD: 0x1001ecd,
    0x1ECE: 0x1001ece,
    0x1ECF: 0x1001ecf,
    0x1ED0: 0x1001ed0,
    0x1ED1: 0x1001ed1,
    0x1ED2: 0x1001ed2,
    0x1ED3: 0x1001ed3,
    0x1ED4: 0x1001ed4,
    0x1ED5: 0x1001ed5,
    0x1ED6: 0x1001ed6,
    0x1ED7: 0x1001ed7,
    0x1ED8: 0x1001ed8,
    0x1ED9: 0x1001ed9,
    0x1EDA: 0x1001eda,
    0x1EDB: 0x1001edb,
    0x1EDC: 0x1001edc,
    0x1EDD: 0x1001edd,
    0x1EDE: 0x1001ede,
    0x1EDF: 0x1001edf,
    0x1EE0: 0x1001ee0,
    0x1EE1: 0x1001ee1,
    0x1EE2: 0x1001ee2,
    0x1EE3: 0x1001ee3,
    0x1EE4: 0x1001ee4,
    0x1EE5: 0x1001ee5,
    0x1EE6: 0x1001ee6,
    0x1EE7: 0x1001ee7,
    0x1EE8: 0x1001ee8,
    0x1EE9: 0x1001ee9,
    0x1EEA: 0x1001eea,
    0x1EEB: 0x1001eeb,
    0x1EEC: 0x1001eec,
    0x1EED: 0x1001eed,
    0x1EEE: 0x1001eee,
    0x1EEF: 0x1001eef,
    0x1EF0: 0x1001ef0,
    0x1EF1: 0x1001ef1,
    0x1EF4: 0x1001ef4,
    0x1EF5: 0x1001ef5,
    0x1EF6: 0x1001ef6,
    0x1EF7: 0x1001ef7,
    0x1EF8: 0x1001ef8,
    0x1EF9: 0x1001ef9,
    0x01A0: 0x10001a0,
    0x01A1: 0x10001a1,
    0x01AF: 0x10001af,
    0x01B0: 0x10001b0,
    0x20A0: 0x10020a0,
    0x20A1: 0x10020a1,
    0x20A2: 0x10020a2,
    0x20A3: 0x10020a3,
    0x20A4: 0x10020a4,
    0x20A5: 0x10020a5,
    0x20A6: 0x10020a6,
    0x20A7: 0x10020a7,
    0x20A8: 0x10020a8,
    0x20A9: 0x10020a9,
    0x20AA: 0x10020aa,
    0x20AB: 0x10020ab,
    0x20AC: 0x20ac,
    0x2070: 0x1002070,
    0x2074: 0x1002074,
    0x2075: 0x1002075,
    0x2076: 0x1002076,
    0x2077: 0x1002077,
    0x2078: 0x1002078,
    0x2079: 0x1002079,
    0x2080: 0x1002080,
    0x2081: 0x1002081,
    0x2082: 0x1002082,
    0x2083: 0x1002083,
    0x2084: 0x1002084,
    0x2085: 0x1002085,
    0x2086: 0x1002086,
    0x2087: 0x1002087,
    0x2088: 0x1002088,
    0x2089: 0x1002089,
    0x2202: 0x1002202,
    0x2205: 0x1002205,
    0x2208: 0x1002208,
    0x2209: 0x1002209,
    0x220B: 0x100220B,
    0x221A: 0x100221A,
    0x221B: 0x100221B,
    0x221C: 0x100221C,
    0x222C: 0x100222C,
    0x222D: 0x100222D,
    0x2235: 0x1002235,
    0x2245: 0x1002248,
    0x2247: 0x1002247,
    0x2262: 0x1002262,
    0x2263: 0x1002263,
    0x2800: 0x1002800,
    0x2801: 0x1002801,
    0x2802: 0x1002802,
    0x2803: 0x1002803,
    0x2804: 0x1002804,
    0x2805: 0x1002805,
    0x2806: 0x1002806,
    0x2807: 0x1002807,
    0x2808: 0x1002808,
    0x2809: 0x1002809,
    0x280a: 0x100280a,
    0x280b: 0x100280b,
    0x280c: 0x100280c,
    0x280d: 0x100280d,
    0x280e: 0x100280e,
    0x280f: 0x100280f,
    0x2810: 0x1002810,
    0x2811: 0x1002811,
    0x2812: 0x1002812,
    0x2813: 0x1002813,
    0x2814: 0x1002814,
    0x2815: 0x1002815,
    0x2816: 0x1002816,
    0x2817: 0x1002817,
    0x2818: 0x1002818,
    0x2819: 0x1002819,
    0x281a: 0x100281a,
    0x281b: 0x100281b,
    0x281c: 0x100281c,
    0x281d: 0x100281d,
    0x281e: 0x100281e,
    0x281f: 0x100281f,
    0x2820: 0x1002820,
    0x2821: 0x1002821,
    0x2822: 0x1002822,
    0x2823: 0x1002823,
    0x2824: 0x1002824,
    0x2825: 0x1002825,
    0x2826: 0x1002826,
    0x2827: 0x1002827,
    0x2828: 0x1002828,
    0x2829: 0x1002829,
    0x282a: 0x100282a,
    0x282b: 0x100282b,
    0x282c: 0x100282c,
    0x282d: 0x100282d,
    0x282e: 0x100282e,
    0x282f: 0x100282f,
    0x2830: 0x1002830,
    0x2831: 0x1002831,
    0x2832: 0x1002832,
    0x2833: 0x1002833,
    0x2834: 0x1002834,
    0x2835: 0x1002835,
    0x2836: 0x1002836,
    0x2837: 0x1002837,
    0x2838: 0x1002838,
    0x2839: 0x1002839,
    0x283a: 0x100283a,
    0x283b: 0x100283b,
    0x283c: 0x100283c,
    0x283d: 0x100283d,
    0x283e: 0x100283e,
    0x283f: 0x100283f,
    0x2840: 0x1002840,
    0x2841: 0x1002841,
    0x2842: 0x1002842,
    0x2843: 0x1002843,
    0x2844: 0x1002844,
    0x2845: 0x1002845,
    0x2846: 0x1002846,
    0x2847: 0x1002847,
    0x2848: 0x1002848,
    0x2849: 0x1002849,
    0x284a: 0x100284a,
    0x284b: 0x100284b,
    0x284c: 0x100284c,
    0x284d: 0x100284d,
    0x284e: 0x100284e,
    0x284f: 0x100284f,
    0x2850: 0x1002850,
    0x2851: 0x1002851,
    0x2852: 0x1002852,
    0x2853: 0x1002853,
    0x2854: 0x1002854,
    0x2855: 0x1002855,
    0x2856: 0x1002856,
    0x2857: 0x1002857,
    0x2858: 0x1002858,
    0x2859: 0x1002859,
    0x285a: 0x100285a,
    0x285b: 0x100285b,
    0x285c: 0x100285c,
    0x285d: 0x100285d,
    0x285e: 0x100285e,
    0x285f: 0x100285f,
    0x2860: 0x1002860,
    0x2861: 0x1002861,
    0x2862: 0x1002862,
    0x2863: 0x1002863,
    0x2864: 0x1002864,
    0x2865: 0x1002865,
    0x2866: 0x1002866,
    0x2867: 0x1002867,
    0x2868: 0x1002868,
    0x2869: 0x1002869,
    0x286a: 0x100286a,
    0x286b: 0x100286b,
    0x286c: 0x100286c,
    0x286d: 0x100286d,
    0x286e: 0x100286e,
    0x286f: 0x100286f,
    0x2870: 0x1002870,
    0x2871: 0x1002871,
    0x2872: 0x1002872,
    0x2873: 0x1002873,
    0x2874: 0x1002874,
    0x2875: 0x1002875,
    0x2876: 0x1002876,
    0x2877: 0x1002877,
    0x2878: 0x1002878,
    0x2879: 0x1002879,
    0x287a: 0x100287a,
    0x287b: 0x100287b,
    0x287c: 0x100287c,
    0x287d: 0x100287d,
    0x287e: 0x100287e,
    0x287f: 0x100287f,
    0x2880: 0x1002880,
    0x2881: 0x1002881,
    0x2882: 0x1002882,
    0x2883: 0x1002883,
    0x2884: 0x1002884,
    0x2885: 0x1002885,
    0x2886: 0x1002886,
    0x2887: 0x1002887,
    0x2888: 0x1002888,
    0x2889: 0x1002889,
    0x288a: 0x100288a,
    0x288b: 0x100288b,
    0x288c: 0x100288c,
    0x288d: 0x100288d,
    0x288e: 0x100288e,
    0x288f: 0x100288f,
    0x2890: 0x1002890,
    0x2891: 0x1002891,
    0x2892: 0x1002892,
    0x2893: 0x1002893,
    0x2894: 0x1002894,
    0x2895: 0x1002895,
    0x2896: 0x1002896,
    0x2897: 0x1002897,
    0x2898: 0x1002898,
    0x2899: 0x1002899,
    0x289a: 0x100289a,
    0x289b: 0x100289b,
    0x289c: 0x100289c,
    0x289d: 0x100289d,
    0x289e: 0x100289e,
    0x289f: 0x100289f,
    0x28a0: 0x10028a0,
    0x28a1: 0x10028a1,
    0x28a2: 0x10028a2,
    0x28a3: 0x10028a3,
    0x28a4: 0x10028a4,
    0x28a5: 0x10028a5,
    0x28a6: 0x10028a6,
    0x28a7: 0x10028a7,
    0x28a8: 0x10028a8,
    0x28a9: 0x10028a9,
    0x28aa: 0x10028aa,
    0x28ab: 0x10028ab,
    0x28ac: 0x10028ac,
    0x28ad: 0x10028ad,
    0x28ae: 0x10028ae,
    0x28af: 0x10028af,
    0x28b0: 0x10028b0,
    0x28b1: 0x10028b1,
    0x28b2: 0x10028b2,
    0x28b3: 0x10028b3,
    0x28b4: 0x10028b4,
    0x28b5: 0x10028b5,
    0x28b6: 0x10028b6,
    0x28b7: 0x10028b7,
    0x28b8: 0x10028b8,
    0x28b9: 0x10028b9,
    0x28ba: 0x10028ba,
    0x28bb: 0x10028bb,
    0x28bc: 0x10028bc,
    0x28bd: 0x10028bd,
    0x28be: 0x10028be,
    0x28bf: 0x10028bf,
    0x28c0: 0x10028c0,
    0x28c1: 0x10028c1,
    0x28c2: 0x10028c2,
    0x28c3: 0x10028c3,
    0x28c4: 0x10028c4,
    0x28c5: 0x10028c5,
    0x28c6: 0x10028c6,
    0x28c7: 0x10028c7,
    0x28c8: 0x10028c8,
    0x28c9: 0x10028c9,
    0x28ca: 0x10028ca,
    0x28cb: 0x10028cb,
    0x28cc: 0x10028cc,
    0x28cd: 0x10028cd,
    0x28ce: 0x10028ce,
    0x28cf: 0x10028cf,
    0x28d0: 0x10028d0,
    0x28d1: 0x10028d1,
    0x28d2: 0x10028d2,
    0x28d3: 0x10028d3,
    0x28d4: 0x10028d4,
    0x28d5: 0x10028d5,
    0x28d6: 0x10028d6,
    0x28d7: 0x10028d7,
    0x28d8: 0x10028d8,
    0x28d9: 0x10028d9,
    0x28da: 0x10028da,
    0x28db: 0x10028db,
    0x28dc: 0x10028dc,
    0x28dd: 0x10028dd,
    0x28de: 0x10028de,
    0x28df: 0x10028df,
    0x28e0: 0x10028e0,
    0x28e1: 0x10028e1,
    0x28e2: 0x10028e2,
    0x28e3: 0x10028e3,
    0x28e4: 0x10028e4,
    0x28e5: 0x10028e5,
    0x28e6: 0x10028e6,
    0x28e7: 0x10028e7,
    0x28e8: 0x10028e8,
    0x28e9: 0x10028e9,
    0x28ea: 0x10028ea,
    0x28eb: 0x10028eb,
    0x28ec: 0x10028ec,
    0x28ed: 0x10028ed,
    0x28ee: 0x10028ee,
    0x28ef: 0x10028ef,
    0x28f0: 0x10028f0,
    0x28f1: 0x10028f1,
    0x28f2: 0x10028f2,
    0x28f3: 0x10028f3,
    0x28f4: 0x10028f4,
    0x28f5: 0x10028f5,
    0x28f6: 0x10028f6,
    0x28f7: 0x10028f7,
    0x28f8: 0x10028f8,
    0x28f9: 0x10028f9,
    0x28fa: 0x10028fa,
    0x28fb: 0x10028fb,
    0x28fc: 0x10028fc,
    0x28fd: 0x10028fd,
    0x28fe: 0x10028fe,
    0x28ff: 0x10028ff
};

var ON_KEYDOWN = 1 << 0; /* Report on keydown, otherwise wait until keypress  */


var specialKeyTable = {
    // These generate a keyDown and keyPress in Firefox and Opera
    8: [0xFF08, ON_KEYDOWN], // BACKSPACE
    13: [0xFF0D, ON_KEYDOWN], // ENTER

    // This generates a keyDown and keyPress in Opera
    9: [0xFF09, ON_KEYDOWN], // TAB

    27: 0xFF1B, // ESCAPE
    46: 0xFFFF, // DELETE
    36: 0xFF50, // HOME
    35: 0xFF57, // END
    33: 0xFF55, // PAGE_UP
    34: 0xFF56, // PAGE_DOWN
    45: 0xFF63, // INSERT
    37: 0xFF51, // LEFT
    38: 0xFF52, // UP
    39: 0xFF53, // RIGHT
    40: 0xFF54, // DOWN
    16: 0xFFE1, // SHIFT
    17: 0xFFE3, // CONTROL
    18: 0xFFE9, // Left ALT (Mac Command)
    112: 0xFFBE, // F1
    113: 0xFFBF, // F2
    114: 0xFFC0, // F3
    115: 0xFFC1, // F4
    116: 0xFFC2, // F5
    117: 0xFFC3, // F6
    118: 0xFFC4, // F7
    119: 0xFFC5, // F8
    120: 0xFFC6, // F9
    121: 0xFFC7, // F10
    122: 0xFFC8, // F11
    123: 0xFFC9  // F12
};

function getEventKeySym(ev) {
    if (typeof ev.which !== "undefined" && ev.which > 0)
        return ev.which;
    return ev.keyCode;
}

// This is based on the approach from noVNC. We handle
// everything in keydown that we have all info for, and that
// are not safe to pass on to the browser (as it may do something
// with the key. The rest we pass on to keypress so we can get the
// translated keysym.
function getKeysymSpecial(ev) {
    if (ev.keyCode in specialKeyTable) {
        var r = specialKeyTable[ev.keyCode];
        var flags = 0;
        if (typeof r != 'number') {
            flags = r[1];
            r = r[0];
        }
        if (ev.type === 'keydown' || flags & ON_KEYDOWN)
            return r;
    }
    // If we don't hold alt or ctrl, then we should be safe to pass
    // on to keypressed and look at the translated data
    if (!ev.ctrlKey && !ev.altKey)
        return null;

    var keysym = getEventKeySym(ev);

    /* Remap symbols */
    switch (keysym) {
    case 186 : keysym = 59; break; // ; (IE)
    case 187 : keysym = 61; break; // = (IE)
    case 188 : keysym = 44; break; // , (Mozilla, IE)
    case 109 : // - (Mozilla, Opera)
        if (true /* TODO: check if browser is firefox or opera */)
            keysym = 45;
        break;
    case 189 : keysym = 45; break; // - (IE)
    case 190 : keysym = 46; break; // . (Mozilla, IE)
    case 191 : keysym = 47; break; // / (Mozilla, IE)
    case 192 : keysym = 96; break; // ` (Mozilla, IE)
    case 219 : keysym = 91; break; // [ (Mozilla, IE)
    case 220 : keysym = 92; break; // \ (Mozilla, IE)
    case 221 : keysym = 93; break; // ] (Mozilla, IE)
    case 222 : keysym = 39; break; // ' (Mozilla, IE)
    }

    /* Remap shifted and unshifted keys */
    if (!!ev.shiftKey) {
        switch (keysym) {
        case 48 : keysym = 41 ; break; // ) (shifted 0)
        case 49 : keysym = 33 ; break; // ! (shifted 1)
        case 50 : keysym = 64 ; break; // @ (shifted 2)
        case 51 : keysym = 35 ; break; // # (shifted 3)
        case 52 : keysym = 36 ; break; // $ (shifted 4)
        case 53 : keysym = 37 ; break; // % (shifted 5)
        case 54 : keysym = 94 ; break; // ^ (shifted 6)
        case 55 : keysym = 38 ; break; // & (shifted 7)
        case 56 : keysym = 42 ; break; // * (shifted 8)
        case 57 : keysym = 40 ; break; // ( (shifted 9)
        case 59 : keysym = 58 ; break; // : (shifted `)
        case 61 : keysym = 43 ; break; // + (shifted ;)
        case 44 : keysym = 60 ; break; // < (shifted ,)
        case 45 : keysym = 95 ; break; // _ (shifted -)
        case 46 : keysym = 62 ; break; // > (shifted .)
        case 47 : keysym = 63 ; break; // ? (shifted /)
        case 96 : keysym = 126; break; // ~ (shifted `)
        case 91 : keysym = 123; break; // { (shifted [)
        case 92 : keysym = 124; break; // | (shifted \)
        case 93 : keysym = 125; break; // } (shifted ])
        case 39 : keysym = 34 ; break; // " (shifted ')
        }
    } else if ((keysym >= 65) && (keysym <=90)) {
        /* Remap unshifted A-Z */
        keysym += 32;
    } else if (ev.keyLocation === 3) {
        // numpad keys
        switch (keysym) {
        case 96 : keysym = 48; break; // 0
        case 97 : keysym = 49; break; // 1
        case 98 : keysym = 50; break; // 2
        case 99 : keysym = 51; break; // 3
        case 100: keysym = 52; break; // 4
        case 101: keysym = 53; break; // 5
        case 102: keysym = 54; break; // 6
        case 103: keysym = 55; break; // 7
        case 104: keysym = 56; break; // 8
        case 105: keysym = 57; break; // 9
        case 109: keysym = 45; break; // -
        case 110: keysym = 46; break; // .
        case 111: keysym = 47; break; // /
        }
    }

    return keysym;
}

/* Translate DOM keyPress event to keysym value */
function getKeysym(ev) {
    var keysym, msg;

    keysym = getEventKeySym(ev);

    if ((keysym > 255) && (keysym < 0xFF00)) {
        // Map Unicode outside Latin 1 to gdk keysyms
        keysym = unicodeTable[keysym];
        if (typeof keysym === 'undefined')
            keysym = 0;
    }

    return keysym;
}

function copyKeyEvent(ev) {
    var members = ['type', 'keyCode', 'charCode', 'which',
                   'altKey', 'ctrlKey', 'shiftKey',
                   'keyLocation', 'keyIdentifier'], i, obj = {};
    for (i = 0; i < members.length; i++) {
        if (typeof ev[members[i]] !== "undefined")
            obj[members[i]] = ev[members[i]];
    }
    return obj;
}

function pushKeyEvent(fev) {
    keyDownList.push(fev);
}

function copyInputEvent(ev) {
    var members = ['inputType', 'data'], i, obj = {};
    for (i = 0; i < members.length; i++) {
	if (typeof ev[members[i]] !== "undefined")
	    obj[members[i]] = ev[members[i]];
    }
    return obj;
}

function pushInputEvent(fev) {
    inputList.push(fev);
}

function getKeyEvent(keyCode, pop) {
    var i, fev = null;
    for (i = keyDownList.length-1; i >= 0; i--) {
        if (keyDownList[i].keyCode === keyCode) {
            if ((typeof pop !== "undefined") && pop)
                fev = keyDownList.splice(i, 1)[0];
            else
                fev = keyDownList[i];
            break;
        }
    }
    return fev;
}

function ignoreKeyEvent(ev) {
    // Blarg. Some keys have a different keyCode on keyDown vs keyUp
    if (ev.keyCode === 229) {
        // French AZERTY keyboard dead key.
        // Lame thing is that the respective keyUp is 219 so we can't
        // properly ignore the keyUp event
        return true;
    }
    return false;
}

function handleKeyDown(e) {
    var fev = null, ev = (e ? e : window.event), keysym = null, suppress = false;

    noteShiftTaps(ev);

    fev = copyKeyEvent(ev);

    keysym = getKeysymSpecial(ev);
    // Save keysym decoding for use in keyUp
    fev.keysym = keysym;
    if (keysym) {
        // If it is a key or key combination that might trigger
        // browser behaviors or it has no corresponding keyPress
        // event, then send it immediately
        if (!ignoreKeyEvent(ev))
            sendInput(BROADWAY_EVENT_KEY_PRESS, [keysym, lastState]);
        suppress = true;
    }

    if (! ignoreKeyEvent(ev)) {
        // Add it to the list of depressed keys
        pushKeyEvent(fev);
    }

    if (suppress) {
        // Paste (Ctrl+V, and Ctrl+Shift+V which yields the shifted 'V' keysym)
        // must NOT be preventDefaulted: we still forward it to the app (so GTK
        // runs its paste action) but we let the browser fire its native 'paste'
        // event into the hidden textarea, which reads the clipboard without any
        // permission popup.
        if (ev.ctrlKey && !ev.altKey && (keysym === KEYSYM_v || keysym === KEYSYM_V))
            return true;
        // Suppress bubbling/default actions
        return cancelEvent(ev);
    }

    // Allow the event to bubble and become a keyPress event which
    // will have the character code translated
    return true;
}

function handleKeyPress(e) {
    var ev = (e ? e : window.event), kdlen = keyDownList.length, keysym = null;

    if (((ev.which !== "undefined") && (ev.which === 0)) ||
        getKeysymSpecial(ev)) {
        // Firefox and Opera generate a keyPress event even if keyDown
        // is suppressed. But the keys we want to suppress will have
        // either:
        // - the which attribute set to 0
        // - getKeysymSpecial() will identify it
        return cancelEvent(ev);
    }

    keysym = getKeysym(ev);

    // Modify the which attribute in the depressed keys list so
    // that the keyUp event will be able to have the character code
    // translation available.
    if (kdlen > 0) {
        keyDownList[kdlen-1].keysym = keysym;
    } else {
        //log("keyDownList empty when keyPress triggered");
    }

    // Send the translated keysym
    if (keysym > 0) {
        lastKeyPressTime = Date.now();
        /* ev.key is the character this keyPress produced ("a", "я", …); used to
         * match (not just time) the echoing 'beforeinput' on the touch OSK. */
        lastKeyPressText = (typeof ev.key === "string") ? ev.key : "";
        sendInput (BROADWAY_EVENT_KEY_PRESS, [keysym, lastState]);
    }

    // Stop keypress events just in case
    return cancelEvent(ev);
}

function handleKeyUp(e) {
    var fev = null, ev = (e ? e : window.event), i, keysym;

    fev = getKeyEvent(ev.keyCode, true);

    if (fev)
        keysym = fev.keysym;
    else {
        //log("Key event (keyCode = " + ev.keyCode + ") not found on keyDownList");
	if (isAndroidChrome && (ev.keyCode == 229)) {
	    var i, fev = null, len = inputList.length, str;
	    for (i = 0; i < len; i++) {
		fev = inputList[i];
		switch(fev.inputType) {
		case "deleteContentBackward":
		    sendInput(BROADWAY_EVENT_KEY_PRESS, [65288, lastState]);
		    sendInput(BROADWAY_EVENT_KEY_RELEASE, [65288, lastState]);
		    break;
		case "insertText":
		    if (fev.data !== undefined) {
			for (let sym of fev.data) {
			    sendInput(BROADWAY_EVENT_KEY_PRESS, [sym.codePointAt(0), lastState]);
			    sendInput(BROADWAY_EVENT_KEY_RELEASE, [sym.codePointAt(0), lastState]);
			}
		    }
		    break;
		default:
		    break;
		}
	    }
	    inputList.splice(0, len);
	}
        keysym = 0;
    }

    if (keysym > 0)
        sendInput (BROADWAY_EVENT_KEY_RELEASE, [keysym, lastState]);
    return cancelEvent(ev);
}

function handleInput (e)  {
    var fev = null, ev = (e ? e : window.event), keysym = null, suppress = false;

    fev = copyInputEvent(ev);
    pushInputEvent(fev);

    // Stop keypress events just in case
    return cancelEvent(ev);
}

function onKeyDown (ev) {
    updateForEvent(ev);
    return handleKeyDown(ev);
}

function onKeyPress(ev) {
    updateForEvent(ev);
    return handleKeyPress(ev);
}

function onKeyUp (ev) {
    updateForEvent(ev);
    return handleKeyUp(ev);
}

function onInput (ev) {
    updateForEvent(ev);
    return handleInput(ev);
}

function cancelEvent(ev)
{
    ev = ev ? ev : window.event;
    if (ev.stopPropagation)
        ev.stopPropagation();
    if (ev.preventDefault)
        ev.preventDefault();
    ev.cancelBubble = true;
    ev.cancel = true;
    ev.returnValue = false;
    return false;
}

function onMouseWheel(ev)
{
    updateForEvent(ev);
    ev = ev ? ev : window.event;

    /* Ctrl+wheel (and trackpad pinch, which the browser reports as ctrlKey) is
     * the browser's native page-zoom on desktop - leave it entirely alone (no
     * preventDefault, no forward) so zooming keeps working. */
    if (ev.ctrlKey)
        return;

    var id = getSurfaceId(ev);
    var pos = getPositionsFromEvent(ev, id);

    /* Standard 'wheel' carries signed deltaX/deltaY; fall back to legacy
     * DOMMouseScroll (detail) / mousewheel (wheelDelta) for old browsers. */
    var dx = ev.deltaX;
    var dy = ev.deltaY;
    if (dx === undefined && dy === undefined) {
        dy = ev.detail ? ev.detail : -ev.wheelDelta;
        dx = 0;
    }

    /* Pick the dominant axis and forward one discrete GTK scroll in it. A
     * horizontal touchpad swipe goes through as left/right scroll (dir 2/3)
     * rather than letting Firefox turn it into a back/forward navigation
     * gesture - which the preventDefault in cancelEvent suppresses. */
    var dir;
    if (Math.abs(dx) > Math.abs(dy))
        dir = dx > 0 ? 3 : 2;        /* right : left */
    else if (dy != 0)
        dir = dy > 0 ? 1 : 0;        /* down : up */
    else
        return cancelEvent(ev);

    sendInput (BROADWAY_EVENT_SCROLL, [realSurfaceWithMouse, id, pos.rootX, pos.rootY, pos.winX, pos.winY, lastState, dir]);

    return cancelEvent(ev);
}

/* Cancel the in-flight single-finger GTK touch (if any) when a pinch starts, so
 * GTK doesn't keep a dangling sequence AND doesn't read begin->end as a tap that
 * leaks through to the widget under the fingers. Touch type 3 == GDK_TOUCH_CANCEL
 * (gdkeventsource.c) aborts the sequence with no activation, unlike a type-2 end,
 * which a click gesture treats as a tap. */
function endInFlightGtkTouch() {
    if (firstTouchDownId == null)
        return;
    var pt = activeTouches[firstTouchDownId];
    if (pt != null) {
        var origId = touchSurfaceIds[firstTouchDownId];
        if (origId === undefined)
            origId = 0;
        var id = getEffectiveEventTarget (origId);
        var pos = getPositionsFromAbsCoord(pt.x / zoomFactor, pt.y / zoomFactor, id);
        sendInput (BROADWAY_EVENT_TOUCH, [3, id, firstTouchDownId, 1, pos.rootX, pos.rootY, pos.winX, pos.winY, lastState]);
    }
    delete touchSurfaceIds[firstTouchDownId];
    firstTouchDownId = null;
}

function beginPinch() {
    pinchActive = true;
    suppressTouchForward = true;
    pinchStartZoom = zoomFactor;
    pinchStartDist = activeTouchDistance();
    pinchStartMid = activeTouchMidpoint();
    endInFlightGtkTouch();
}

/* Commit the live-previewed zoom: report the new (reflowed, crisper) size so GTK
 * re-renders sharp; the wrapper transform already shows the magnified view. */
function endPinch() {
    pinchActive = false;
    saveZoom();
    sendScreenSizeChanged();
    applyZoomTransform(zoomFactor);
}

function onTouchStart(ev) {
    ev.preventDefault();

    updateKeyboardStatus();
    updateForEvent(ev);

    /* Track every finger for pinch-distance math. */
    for (var i = 0; i < ev.changedTouches.length; i++) {
        var t = ev.changedTouches.item(i);
        activeTouches[t.identifier] = { x: t.pageX, y: t.pageY };
    }

    /* Two or more fingers => pinch-zoom: consume the gesture, don't forward it
     * to GTK as touches. */
    if (activeTouchCount() >= 2) {
        if (!pinchActive)
            beginPinch();
        armHoldMenu();   /* (re)start the two-finger press-and-hold timer */
        return;
    }
    /* A finger left over from a just-ended pinch is never forwarded, so it can't
     * kick off a stray GTK touch. */
    if (suppressTouchForward)
        return;

    for (var i = 0; i < ev.changedTouches.length; i++) {
        var touch = ev.changedTouches.item(i);
	var touchId = touchIdentifierStart(touch.identifier);

        var origId = getSurfaceId(touch);
        touchSurfaceIds[touch.identifier] = origId;
        var id = getEffectiveEventTarget (origId);
        var pos = getPositionsFromEvent(touch, id);
        var isEmulated = 0;

        if (firstTouchDownId == null) {
            firstTouchDownId = touchId;
            isEmulated = 1;

            /* Track the touched surface (grab handling needs it), but emit no
             * ENTER/LEAVE pointer crossings: they ride the mouse device, so GTK
             * would read a tap as mouse hover and show :hover styling and
             * tooltips. GtkTooltip only suppresses tooltips for
             * GDK_SOURCE_TOUCHSCREEN events; the touch events route the tap. */
            surfaceWithMouse = id;
            realSurfaceWithMouse = origId;
        }

        sendInput (BROADWAY_EVENT_TOUCH, [0, id, touchId, isEmulated, pos.rootX, pos.rootY, pos.winX, pos.winY, lastState]);
    }

    /* Touch events stay targeted at the node the touch began on; once GTK
     * repaints that surface the node is detached from the DOM and move/end
     * events stop bubbling to document. Listen on the target itself so the
     * gesture keeps flowing (the document listeners stay as a fallback; the
     * handlers de-dupe). addEventListener de-dupes and detached nodes are
     * garbage-collected, so no teardown is needed. */
    if (ev.target && ev.target.addEventListener) {
        ev.target.addEventListener('touchmove', onTouchMove, {passive: false});
        ev.target.addEventListener('touchend', onTouchEnd, {passive: false});
        ev.target.addEventListener('touchcancel', onTouchEnd, {passive: false});
    }
}

function onTouchMove(ev) {
    /* Reaches us from both the document and the touchstart-target listener. */
    if (ev.broadwayHandled)
        return;
    ev.broadwayHandled = true;
    ev.preventDefault();

    updateKeyboardStatus();
    updateForEvent(ev);

    /* Keep finger positions current for pinch-distance math. */
    for (var i = 0; i < ev.changedTouches.length; i++) {
        var t = ev.changedTouches.item(i);
        if (activeTouches[t.identifier] != null)
            activeTouches[t.identifier] = { x: t.pageX, y: t.pageY };
    }

    if (pinchActive) {
        var dist = activeTouchDistance();
        if (pinchStartDist > 0 && dist > 0) {
            /* Magnify live around the pinch midpoint, floating with the fingers
             * (the bitmap may be briefly soft); the crisp reflow is committed on
             * touch-end. */
            zoomFactor = clampZoom(pinchStartZoom * (dist / pinchStartDist));
            applyPinchPreview();
        }
        holdMenuCheckDrift();   /* finger movement = real pinch/pan, abort hold */
        return;
    }
    if (suppressTouchForward)
        return;

    for (var i = 0; i < ev.changedTouches.length; i++) {
        var touch = ev.changedTouches.item(i);
	var touchId = touchIdentifier(touch.identifier);

        var origId = touchSurfaceIds[touch.identifier];
        if (origId === undefined)
            origId = getSurfaceId(touch);
        var id = getEffectiveEventTarget (origId);
        var pos = getPositionsFromEvent(touch, id);

        var isEmulated = 0;
        if (firstTouchDownId == touchId) {
            isEmulated = 1;
        }

        sendInput (BROADWAY_EVENT_TOUCH, [1, id, touchId, isEmulated, pos.rootX, pos.rootY, pos.winX, pos.winY, lastState]);
    }
}

function onTouchEnd(ev) {
    /* Reaches us from both the document and the touchstart-target listener. */
    if (ev.broadwayHandled)
        return;
    ev.broadwayHandled = true;
    ev.preventDefault();

    updateKeyboardStatus();
    updateForEvent(ev);

    /* Drop lifted fingers from pinch tracking. */
    for (var i = 0; i < ev.changedTouches.length; i++) {
        var t = ev.changedTouches.item(i);
        delete activeTouches[t.identifier];
    }

    /* A lifted finger breaks the two-finger hold. */
    if (activeTouchCount() < 2)
        cancelHoldMenu();

    if (pinchActive) {
        if (activeTouchCount() < 2)
            endPinch();
        return; /* pinch fingers are never forwarded as touches */
    }
    if (suppressTouchForward) {
        /* Pinch already committed but fingers still down: keep suppressing until
         * all lift, so a lingering finger can't start a stray GTK touch. */
        if (activeTouchCount() == 0)
            suppressTouchForward = false;
        return;
    }

    /* A browser `touchcancel` is an aborted touch, NOT a completed tap: forward
     * it as GDK_TOUCH_CANCEL (type 3), not a type-2 end. This is the real
     * pinch-leak fix: mobile Firefox fires `touchcancel` on the first finger
     * when the second lands; if that beats the pinch-begin (touchstart of finger
     * 2) it reaches here while !pinchActive, and a type-2 end would make GTK read
     * begin->end as a tap that activates the widget under the fingers. A type-2
     * end stays only for a genuine `touchend`. */
    var touchType = (ev.type === 'touchcancel') ? 3 : 2;

    for (var i = 0; i < ev.changedTouches.length; i++) {
        var touch = ev.changedTouches.item(i);
	var touchId = touchIdentifier(touch.identifier);

        var origId = touchSurfaceIds[touch.identifier];
        if (origId === undefined)
            origId = getSurfaceId(touch);
        delete touchSurfaceIds[touch.identifier];
        var id = getEffectiveEventTarget (origId);
        var pos = getPositionsFromEvent(touch, id);

        var isEmulated = 0;
        if (firstTouchDownId == touchId) {
            isEmulated = 1;
            firstTouchDownId = null;
        }

        sendInput (BROADWAY_EVENT_TOUCH, [touchType, id, touch.identifier, isEmulated, pos.rootX, pos.rootY, pos.winX, pos.winY, lastState]);
    }
}

/* ---- Debug menu summon triggers ----------------------------------------
 * Two ways to summon the debug menu, both chosen to be inert to any GTK app and
 * safe in any browser:
 *   - keyboard: triple-tap Shift (nothing else between the taps)
 *   - touch:    two-finger press-and-hold for HOLD_MENU_MS
 * Neither costs input latency. A bare Shift is a no-op modifier, so we observe
 * the taps while still forwarding each Shift to GTK as usual. On this backend
 * 2+ fingers already drive pinch-zoom client-side and are never forwarded to
 * GTK, so the hold layers onto the existing pinch path: real pinches/pans move
 * the fingers and trip the slop check (aborting the hold), while single-finger
 * drags never reach here at all.
 *
 * onSummonMenu() is the single fire point: it sends BROADWAY_EVENT_MENU to the
 * daemon, which spawns (or toggles) a native GTK4 menu window on demand. The
 * menu is purely server-side - it composites into the same display, so any
 * connected browser (desktop or mobile) sees it. */

var SHIFT_TAP_MS = 600;        /* max gap between Shift taps */
var shiftTapCount = 0;
var shiftLastTap = 0;

/* Observer only - does not consume the Shift, so no latency and no swallowed
 * modifier. Any non-Shift key, or auto-repeat from a held Shift, breaks the run. */
function noteShiftTaps(ev) {
    if (ev.keyCode !== 16) { shiftTapCount = 0; return; }   /* 16 = Shift (L/R) */
    if (ev.repeat) return;                                  /* ignore held-key repeat */
    var now = Date.now();
    shiftTapCount = (now - shiftLastTap <= SHIFT_TAP_MS) ? shiftTapCount + 1 : 1;
    shiftLastTap = now;
    if (shiftTapCount >= 3) {
        shiftTapCount = 0;
        onSummonMenu();
    }
}

var HOLD_MENU_MS = 2500;       /* "several seconds" of stillness to summon */
var HOLD_MENU_SLOP = 16;       /* px a finger may drift before it counts as a gesture */
var holdMenuTimer = null;
var holdMenuAnchors = null;    /* {identifier: {x,y}} captured when the hold armed */

/* Armed when a second finger lands (alongside beginPinch). We never withhold the
 * fingers from the pinch path, so the common pinch/pan case is unchanged. */
function armHoldMenu() {
    cancelHoldMenu();
    holdMenuAnchors = {};
    for (var id in activeTouches)
        holdMenuAnchors[id] = { x: activeTouches[id].x, y: activeTouches[id].y };
    holdMenuTimer = setTimeout(function () {
        holdMenuTimer = null;
        holdMenuAnchors = null;
        onSummonMenu();
    }, HOLD_MENU_MS);
}

function cancelHoldMenu() {
    if (holdMenuTimer) {
        clearTimeout(holdMenuTimer);
        holdMenuTimer = null;
    }
    holdMenuAnchors = null;
}

/* Any finger drifting past the slop means a real pinch/pan, not a hold: abort. */
function holdMenuCheckDrift() {
    if (!holdMenuAnchors)
        return;
    for (var id in activeTouches) {
        var a = holdMenuAnchors[id];
        if (!a) { cancelHoldMenu(); return; }   /* finger set changed */
        var dx = activeTouches[id].x - a.x;
        var dy = activeTouches[id].y - a.y;
        if (dx * dx + dy * dy > HOLD_MENU_SLOP * HOLD_MENU_SLOP) {
            cancelHoldMenu();
            return;
        }
    }
}

function onSummonMenu() {
    sendInput(BROADWAY_EVENT_MENU, []);
}

/* Heuristic for phones/tablets, where focusing an offscreen input summons the
 * on-screen keyboard (wanted for GTK entries) and focusing a textarea would do
 * so unwantedly (so the clipboard capture textarea is skipped there). */
function isTouchDevice()
{
    return (window.matchMedia && window.matchMedia("(pointer: coarse)").matches) ||
           /(iPad|iPhone|iPod|Android)/i.test(navigator.userAgent);
}

function setupDocument(document)
{
    document.oncontextmenu = function () { return false; };
    document.onmousemove = onMouseMove;
    document.onmouseover = onMouseOver;
    document.onmouseout = onMouseOut;
    document.onmousedown = onMouseDown;
    document.onmouseup = onMouseUp;
    document.onkeydown = onKeyDown;
    document.onkeypress = onKeyPress;
    document.onkeyup = onKeyUp;

    if (document.addEventListener) {
      /* Standard wheel event (covers vertical + horizontal). passive:false so
       * preventDefault() actually fires - needed to stop Firefox turning a
       * horizontal touchpad swipe into a back/forward navigation gesture. */
      document.addEventListener('wheel', onMouseWheel, {passive: false});
      /* passive:false is required or the browser ignores preventDefault() in
       * the handlers (touch listeners on the document default to passive), and
       * Firefox/Android then cancels the touch on the slightest move - turning
       * taps into touchcancel and eating roughly half of them. */
      document.addEventListener('touchstart', onTouchStart, passiveSupported ? { passive: false, capture: false } : false);
      document.addEventListener('touchmove', onTouchMove, passiveSupported ? { passive: false, capture: false } : false);
      document.addEventListener('touchend', onTouchEnd, passiveSupported ? { passive: false, capture: false } : false);
      /* Still handle touchcancel (system-initiated): end the sequence like a
       * normal touchend so firstTouchDownId and the GTK grab aren't stranded. */
      document.addEventListener('touchcancel', onTouchEnd, passiveSupported ? { passive: false, capture: false } : false);
    } else if (document.attachEvent) {
      element.attachEvent("onmousewheel", onMouseWheel);
    }

    /* Non-touch only: keep a hidden textarea focused to capture native 'paste'
     * (Ctrl+V / menu Paste) events. On touch devices (phones/tablets) a focused
     * textarea would pop up the on-screen keyboard, so skip it there. A
     * fine-pointer touch laptop still keeps the feature. */
    if (!isTouchDevice() && document.addEventListener) {
        clipboardArea = document.createElement("textarea");
        clipboardArea.setAttribute("autocapitalize", "off");
        clipboardArea.setAttribute("autocomplete", "off");
        clipboardArea.setAttribute("autocorrect", "off");
        clipboardArea.spellcheck = false;
        clipboardArea.style.position = "absolute";
        clipboardArea.style.left = "-1000px";
        clipboardArea.style.top = "-1000px";
        clipboardArea.style.width = "1px";
        clipboardArea.style.height = "1px";
        clipboardArea.style.opacity = "0";
        document.body.appendChild(clipboardArea);

        clipboardArea.addEventListener("paste", function (ev) {
            var t = "";
            if (ev.clipboardData && ev.clipboardData.getData)
                t = ev.clipboardData.getData("text/plain");
            else if (window.clipboardData && window.clipboardData.getData)
                t = window.clipboardData.getData("Text");
            pasteCache = (t == null) ? "" : t;
            pasteCacheTime = Date.now();
            /* Cache only; never insert into the hidden textarea. */
            ev.preventDefault();
        });
        /* Refocus the capture textarea when the user interacts with the app or
         * returns to the tab, so the next paste is captured here - without
         * holding focus hostage from other elements via a tight blur loop.
         * (Desktop IME composition could still land here; rare for canvas apps.) */
        document.addEventListener("mousedown", function () {
            if (clipboardArea) clipboardArea.focus();
        });
        window.addEventListener("focus", function () {
            if (clipboardArea) clipboardArea.focus();
        });
        clipboardArea.focus();
    }
}

function sendScreenSizeChanged() {
    var w, h, s;
    /* Touch pinch-zoom: report a logical size shrunk by zoomFactor and a
     * render scale bumped by it, so GTK re-lays-out smaller and renders crisper;
     * the #zoomRoot transform then magnifies that back to fill the viewport.
     * At zoom 1 this is byte-identical to the unmodified behaviour (desktop,
     * which gets its own crisp zoom from the browser's native page-zoom). */
    w = Math.round(window.innerWidth / zoomFactor);
    h = Math.round(window.innerHeight / zoomFactor);
    s = Math.max(1, Math.round(window.devicePixelRatio * zoomFactor));
    sendInput (BROADWAY_EVENT_SCREEN_SIZE_CHANGED, [w, h, s]);
}

function start()
{
    zoomRoot = document.getElementById('zoomRoot') || document.body;
    /* Restore a zoom saved from a previous session before the initial
     * sendScreenSizeChanged() below, so GTK lays out at the right size at once. */
    loadSavedZoom();
    applyZoomTransform(zoomFactor);

    setupDocument(document);

    /* Recovery triggers: visibility/pageshow reconnect if down, online forces
     * it, offline shows the overlay. Heartbeat covers what fires none of these. */
    document.addEventListener("visibilitychange", function () {
        if (document.visibilityState === "visible")
            onVisible();
    });
    window.addEventListener("pageshow", onVisible);
    window.addEventListener("online", reconnectNow);
    window.addEventListener("offline", handleConnectionLost);
    startHeartbeat();

    window.onresize = function(ev) {
        sendScreenSizeChanged();
    };
    window.matchMedia('screen and (min-resolution: 2dppx)').
        addListener(function(e) {
                        if (e.matches) {
                            /* devicePixelRatio >= 2 */
                            sendScreenSizeChanged();
                        } else {
                            /* devicePixelRatio < 2 */
                            sendScreenSizeChanged();
                        }
                    });
    sendScreenSizeChanged();
}

/* ---- Auto-reconnect + session liveness --------------------------------
 * Mobile browsers suspend the WebSocket on screen-off / network change. We
 * reconnect in place (no reload), keeping the last frame dimmed under a spinner
 * until the resync repaints. The daemon's SESSION token tells us whether it is
 * the same session (resume) or restarted; an unchanged token resumes, a changed
 * token or DISCONNECTED shows the disconnected icon until refresh. */

var RECONNECT_SPINNER_SVG =
    '<svg width="64" height="64" viewBox="0 0 50 50" aria-label="reconnecting">' +
    '<circle cx="25" cy="25" r="20" fill="none" stroke="rgba(255,255,255,0.25)" stroke-width="5"/>' +
    '<path d="M25 5 a20 20 0 0 1 0 40" fill="none" stroke="#fff" stroke-width="5" stroke-linecap="round">' +
    '<animateTransform attributeName="transform" type="rotate" from="0 25 25" to="360 25 25" dur="0.9s" repeatCount="indefinite"/>' +
    '</path></svg>';

var DISCONNECTED_SVG =
    '<svg width="64" height="64" viewBox="0 0 50 50" aria-label="disconnected">' +
    '<circle cx="25" cy="25" r="20" fill="none" stroke="#fff" stroke-width="5"/>' +
    '<line x1="12" y1="12" x2="38" y2="38" stroke="#fff" stroke-width="5" stroke-linecap="round"/>' +
    '</svg>';

function showOverlay(kind)
{
    if (!overlayEl) {
        overlayEl = document.createElement("div");
        var s = overlayEl.style;
        s.position = "fixed";
        s.left = s.top = s.right = s.bottom = "0";
        s.width = "100vw";
        s.height = "100vh";
        s.background = "rgba(0,0,0,0.66)";
        s.zIndex = "2147483647";
        s.display = "flex";
        s.alignItems = "center";
        s.justifyContent = "center";
        s.pointerEvents = "auto";   /* swallow input while overlaid */
        document.body.appendChild(overlayEl);
    }
    overlayEl.innerHTML = (kind === "disconnected") ? DISCONNECTED_SVG : RECONNECT_SPINNER_SVG;
}

function hideOverlay()
{
    if (overlayEl && overlayEl.parentNode)
        overlayEl.parentNode.removeChild(overlayEl);
    overlayEl = null;
}

/* Drop all surfaces/textures so the daemon's resync can rebuild from scratch.
 * cmdCreateSurface is not idempotent, so we must clear before NEW_SURFACE. */
function resetClientState()
{
    for (var id in surfaces) {
        var s = surfaces[id];
        if (s && s.div && s.div.parentNode)
            s.div.parentNode.removeChild(s.div);
    }
    surfaces = {};
    stackingOrder = [];
    for (var tid in textures) {
        var t = textures[tid];
        if (t && t.url && t.url.indexOf("blob") == 0)
            window.URL.revokeObjectURL(t.url);
    }
    textures = {};
    surfaceWithMouse = 0;
    realSurfaceWithMouse = 0;
    firstTouchDownId = null;
    activeTouches = {};
}

function onSessionToken(token, cid)
{
    clientId = cid;

    var firstTime = (sessionToken === null);
    if (firstTime)
        sessionToken = token;

    if (firstTime || token === sessionToken) {
        /* Same session (or first connect): resume in place. */
        if (reconnecting) {
            if (!firstTime)
                resetClientState();
            awaitFirstFrame = true;    /* overlay drops on the first repaint */
            /* Keep a single timer so a stale resume can't strand the spinner. */
            if (resumeSafetyTimer)
                clearTimeout(resumeSafetyTimer);
            resumeSafetyTimer = setTimeout(function () {
                resumeSafetyTimer = null;
                if (reconnecting) {
                    reconnecting = false;
                    awaitFirstFrame = false;
                    hideOverlay();
                }
            }, 3000);
        }
        return;
    }

    invalidateSession();               /* token changed -> daemon restarted */
}

function invalidateSession()
{
    sessionInvalidated = true;
    reconnecting = false;
    awaitFirstFrame = false;
    inputSocket = null;
    if (reconnectTimer) {
        clearTimeout(reconnectTimer);
        reconnectTimer = null;
    }
    if (resumeSafetyTimer) {
        clearTimeout(resumeSafetyTimer);
        resumeSafetyTimer = null;
    }
    /* Close the socket so we don't linger on the daemon. */
    if (ws) {
        try {
            ws.onopen = ws.onclose = ws.onerror = ws.onmessage = null;
            ws.close();
        } catch (e) { }
        ws = null;
    }
    /* Drop queued messages; the pending rAF nulls outstandingDisplayCommands. */
    outstandingCommands.length = 0;
    showOverlay("disconnected");
}

function scheduleReconnect()
{
    if (sessionInvalidated || reconnectTimer)
        return;
    reconnectDelay = reconnectDelay ? Math.min(reconnectDelay * 2, 4000) : 250;
    reconnectTimer = setTimeout(function () {
        reconnectTimer = null;
        connect();
    }, reconnectDelay);
}

/* Link is (probably) gone: show the overlay and start retrying. Covers onclose,
 * the offline event, and a stale heartbeat (socket may still report OPEN). */
function handleConnectionLost()
{
    if (sessionInvalidated)
        return;
    inputSocket = null;
    if (!reconnecting) {
        reconnecting = true;
        showOverlay("reconnecting");
    }
    /* Drop the possibly half-open socket so the next connect() is clean. */
    if (ws) {
        try {
            ws.onopen = ws.onclose = ws.onerror = ws.onmessage = null;
            ws.close();
        } catch (e) { }
        ws = null;
    }
    scheduleReconnect();
}

/* Force an immediate attempt now (network came back / tab foregrounded). */
function reconnectNow()
{
    if (sessionInvalidated)
        return;
    if (reconnectTimer) {
        clearTimeout(reconnectTimer);
        reconnectTimer = null;
    }
    reconnectDelay = 0;
    if (!reconnecting) {
        reconnecting = true;
        showOverlay("reconnecting");
    }
    connect();   /* connect() tears down any existing socket first */
}

/* Tab visible again: reset lastPongTime so a stale background value doesn't trip
 * the heartbeat, then reconnect only if the link looks down. */
function onVisible()
{
    lastPongTime = Date.now();
    reconnectIfDown();
}

/* Reconnect only if the link looks down, so a tab switch on a healthy socket
 * doesn't force a needless resync. */
function reconnectIfDown()
{
    if (sessionInvalidated)
        return;
    if (!navigator.onLine) {
        handleConnectionLost();
        return;
    }
    if (!ws || ws.readyState !== WebSocket.OPEN || inputSocket == null)
        reconnectNow();
}

function startHeartbeat()
{
    if (heartbeatTimer)
        return;
    lastPongTime = Date.now();
    heartbeatTimer = setInterval(heartbeatTick, HEARTBEAT_MS);
}

function heartbeatTick()
{
    if (sessionInvalidated)
        return;
    if (document.hidden)           /* screen off: timers freeze anyway */
        return;
    if (reconnecting && (!ws || ws.readyState !== WebSocket.OPEN))
        return;                    /* still reopening: let the retry loop drive */
    if (!navigator.onLine ||
        !ws || ws.readyState !== WebSocket.OPEN || inputSocket == null) {
        handleConnectionLost();
        return;
    }
    /* No PONG in a while: dead link, or a socket that opened but never got its
     * SESSION (data path wedged with no onclose). */
    if (Date.now() - lastPongTime > HEARTBEAT_TIMEOUT_MS) {
        handleConnectionLost();
        return;
    }
    if (!reconnecting) {
        pingSentTime = Date.now();
        sendInput(BROADWAY_EVENT_PING, [lastRtt]);
    }
}

function connect()
{
    var url = window.location.toString();
    var query_string = url.split("?");
    if (query_string.length > 1) {
        var params = query_string[1].split("&");

        for (var i=0; i<params.length; i++) {
            var pair = params[i].split("=");
            if (pair[0] == "debug" && pair[1] == "decoding")
                debugDecoding = true;
        }
    }

    var loc = window.location.toString().replace("http:", "ws:").replace("https:", "wss:");
    loc = loc.substr(0, loc.lastIndexOf('/')) + "/socket";
    /* cid lets the daemon resume us if we still own the display; a fresh load
     * (clientId 0) omits it and takes over. */
    if (clientId)
        loc += (loc.indexOf("?") < 0 ? "?" : "&") + "cid=" + clientId;

    /* Drop any previous socket without letting its onclose drive a second
     * reconnect (e.g. reconnectNow() pre-empting a suspended one). */
    if (ws) {
        try {
            ws.onopen = ws.onclose = ws.onerror = ws.onmessage = null;
            ws.close();
        } catch (e) { }
    }

    ws = new WebSocket(loc, "broadway");
    ws.binaryType = "arraybuffer";

    ws.onopen = function() {
        inputSocket = ws;
        reconnectDelay = 0;        /* reset backoff on a successful open */
        lastPongTime = Date.now(); /* fresh socket: don't immediately time out */
    };
    ws.onerror = function() {
        /* onclose follows and drives the reconnect. */
    };
    ws.onclose = function() {
        inputSocket = null;
        if (sessionInvalidated)
            return;
        reconnecting = true;
        showOverlay("reconnecting");
        scheduleReconnect();
    };
    ws.onmessage = function(event) {
        handleMessage(event.data);
    };

    /* On touch devices, summon the on-screen keyboard by focusing this hidden
     * input when GTK asks for keyboard input (see updateKeyboardStatus). Not
     * iOS-only: Android needs it too, or the OSK never appears.
     * Guarded against reconnects creating a second input. */
    if (isTouchDevice() && fakeInput == null) {
        fakeInput = document.createElement("input");
        fakeInput.type = "text";
        fakeInput.setAttribute("autocapitalize", "off");
        fakeInput.setAttribute("autocomplete", "off");
        fakeInput.setAttribute("autocorrect", "off");
        fakeInput.spellcheck = false;
        fakeInput.style.position = "absolute";
        fakeInput.style.left = "-1000px";
        fakeInput.style.top = "-1000px";
        document.body.appendChild(fakeInput);

        /* The OSK input doubles as the clipboard surface on touch: while focused
         * it catches a system paste (e.g. Gboard's clipboard chip) as a native
         * 'paste'/'beforeinput', and serves as copyViaTextarea()'s copy target. */
        clipboardArea = fakeInput;
        fakeInput.addEventListener("paste", function (ev) {
            var t = "";
            if (ev.clipboardData && ev.clipboardData.getData)
                t = ev.clipboardData.getData("text/plain");
            capturePaste(t);
            ev.preventDefault();
        });
        fakeInput.addEventListener("beforeinput", function (ev) {
            if (ev.inputType === "insertFromPaste") {
                /* Skip if the 'paste' event already handled this. */
                if (Date.now() - lastPasteForwardTime < INPUT_ECHO_WINDOW_MS)
                    return;
                var t = ev.data;
                if (t == null && ev.dataTransfer && ev.dataTransfer.getData)
                    t = ev.dataTransfer.getData("text/plain");
                capturePaste(t);
                ev.preventDefault();
                return;
            }
            /* Typed text that produced no usable keyPress (non-Latin layouts,
             * autocorrect/suggestion replacements, dead keys): GBoard reports a
             * keyCode-229 keydown we drop, and the character arrives only here.
             * Composition (insertCompositionText) is handled on compositionend. */
            if (imeComposing)
                return;
            if (ev.inputType === "insertText" ||
                ev.inputType === "insertReplacementText") {
                /* Latin keys fire both keyPress and beforeinput; don't re-send the
                 * same char the keyPress already forwarded (matched by value, so a
                 * genuinely different char is never suppressed). */
                if (ev.data === lastKeyPressText &&
                    Date.now() - lastKeyPressTime < INPUT_ECHO_WINDOW_MS)
                    return;
                /* Likewise for a composition just committed via compositionend. */
                if (ev.data === lastCompositionText &&
                    Date.now() - lastCompositionEndTime < INPUT_ECHO_WINDOW_MS)
                    return;
                if (ev.data) {
                    commitTextToGtk(ev.data);
                    fakeInput.value = "";
                }
                ev.preventDefault();
            }
        });
        /* IME composition (CJK, gesture typing): commit the final string once,
         * on compositionend — Broadway has no preedit, so intermediate updates
         * are not forwarded. */
        fakeInput.addEventListener("compositionstart", function () {
            imeComposing = true;
        });
        fakeInput.addEventListener("compositionend", function (ev) {
            imeComposing = false;
            lastCompositionEndTime = Date.now();
            lastCompositionText = ev.data || "";
            if (ev.data)
                commitTextToGtk(ev.data);
            fakeInput.value = "";
        });
    }
}
