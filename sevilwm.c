/* evilwm - Minimalist Window Manager for X
 * Copyright (C) 1999-2000 Ciaran Anscomb <ciarana@rd.bbc.co.uk>
 * see README for license and other details. */

#include "evilwm.h"
#include "events.h"
#include "screen.h"
#include "keys.h"

#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <X11/cursorfont.h>
#include <X11/Xlib.h>
#include <X11/Xproto.h>

const unsigned int sevilwm_key_event_modifiers = ControlMask | Mod1Mask;

Display     *dpy;
Client      *current = NULL;
int     screen;
Window      root;
GC      invert_gc;
XFontStruct *font;
Client      *head_client;
Atom        xa_wm_state;
Atom        xa_wm_change_state;
Atom        xa_wm_protos;
Atom        xa_wm_delete;
Atom        xa_wm_cmapwins;
Cursor      move_curs;
Cursor      resize_curs;
char        *opt_display = "";
const char  *opt_font = DEF_FONT;
const char  *opt_fg = DEF_FG;
const char  *opt_bg = DEF_BG;
int     opt_vd = DEF_VD;
int     opt_bw = DEF_BW;
XColor      fg, bg, fc;
char        *opt_fc = DEF_FC;
int     vdesk = 1;
int wm_running;

char throwUnmaps;

Client **prev_focused;

void cleanup() {
    while(head_client) remove_client(head_client, QUITTING);
    XSetInputFocus(dpy, PointerRoot, RevertToPointerRoot, CurrentTime);
    XInstallColormap(dpy, DefaultColormap(dpy, screen));
    XCloseDisplay(dpy);
}

void quit_nicely() {
    cleanup();
    exit(0);
}

void handle_signal(int signo) {
    if (signo == SIGCHLD) {
        wait(NULL);
        return;
    }

    quit_nicely();
}
void throwAllUnmapEvent() {
    XEvent ev;
    // throw all event
    while (XCheckTypedEvent(dpy, UnmapNotify, &ev)) {}
}

int handle_xerror(Display *d, XErrorEvent *e) {
    Client *c = find_client(e->resourceid);

    /* if (e->error_code == BadAccess && e->resourceid == root) { */
    if (e->error_code == BadAccess &&
        e->request_code == X_ChangeWindowAttributes) {
        exit(1);
    }
    fprintf(stderr, "XError %x %d ", e->error_code, e->request_code);
    if (c && e->error_code != BadWindow) {
        remove_client(c, NOT_QUITTING);
    }
    return 0;
}

int ignore_xerror(Display *d, XErrorEvent *e) {
    return 0;
}


void force_set_focus(void);

void setup_display() {
    XGCValues gv;
    XSetWindowAttributes attr;
    XColor dummy;

    dpy = XOpenDisplay(opt_display);
    if (!dpy) {
        exit(1);
    }
    XSetErrorHandler(handle_xerror);

    screen = DefaultScreen(dpy);
    root = RootWindow(dpy, screen);

    xa_wm_state = XInternAtom(dpy, "WM_STATE", False);
    xa_wm_change_state = XInternAtom(dpy, "WM_CHANGE_STATE", False);
    xa_wm_protos = XInternAtom(dpy, "WM_PROTOCOLS", False);
    xa_wm_delete = XInternAtom(dpy, "WM_DELETE_WINDOW", False);

    XAllocNamedColor(dpy, DefaultColormap(dpy, screen), opt_fg, &fg, &dummy);
    XAllocNamedColor(dpy, DefaultColormap(dpy, screen), opt_bg, &bg, &dummy);
    XAllocNamedColor(dpy, DefaultColormap(dpy, screen), opt_fc, &fc, &dummy);

    font = XLoadQueryFont(dpy, opt_font);
    if (!font) font = XLoadQueryFont(dpy, "*");

    move_curs = XCreateFontCursor(dpy, XC_fleur);
    resize_curs = XCreateFontCursor(dpy, XC_plus);

    gv.function = GXinvert;
    gv.subwindow_mode = IncludeInferiors;
    gv.line_width = 1;  /* opt_bw */
    gv.font = font->fid;
    invert_gc = XCreateGC(dpy, root, GCFunction | GCSubwindowMode | GCLineWidth | GCFont, &gv);

    attr.event_mask = ChildMask | PropertyChangeMask
        | ButtonMask
        ;
    XChangeWindowAttributes(dpy, root, CWEventMask, &attr);
    /* Unfortunately grabbing AnyKey under Solaris seems not to work */
    /* XGrabKey(dpy, AnyKey, ControlMask|Mod1Mask, root, True, GrabModeAsync, GrabModeAsync); */
    /* So now I grab each and every one. */
    XGrabKey(dpy, AnyKey, sevilwm_key_event_modifiers,
             root, True, GrabModeAsync, GrabModeAsync);
    XGrabKey(dpy, AnyKey, sevilwm_key_event_modifiers | ShiftMask,
             root, True, GrabModeAsync, GrabModeAsync);

    XGrabKey(dpy, XKeysymToKeycode(dpy, XK_Tab), Mod1Mask, root, True, GrabModeAsync, GrabModeAsync);
}

void scan_windows() {
    unsigned int i, nwins;
    Window dw1, dw2, *wins;
    XWindowAttributes attr;

    XQueryTree(dpy, root, &dw1, &dw2, &wins, &nwins);
    for (i = 0; i < nwins; i++) {
        XGetWindowAttributes(dpy, wins[i], &attr);
        if (!attr.override_redirect && attr.map_state == IsViewable)
            make_new_client(wins[i]);
    }
    XFree(wins);
}

int main(int argc, char *argv[]) {
    struct sigaction act;
    int i;
    XEvent ev;

    for (i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-fn") && i+1<argc)
            opt_font = argv[++i];
        else if (!strcmp(argv[i], "-display") && i+1<argc)
            opt_display = argv[++i];
        else if (!strcmp(argv[i], "-fg") && i+1<argc)
            opt_fg = argv[++i];
        else if (!strcmp(argv[i], "-bg") && i+1<argc)
            opt_bg = argv[++i];
        else if (!strcmp(argv[i], "-fc") && i+1<argc)
            opt_fc = argv[++i];
        else if (!strcmp(argv[i], "-bw") && i+1<argc)
            opt_bw = atoi(argv[++i]);
        else if (!strcmp(argv[i], "-vd") && i+1<argc) {
            opt_vd = atoi(argv[++i]);
        } else {
            printf("usage: evilwm [-display display] [-vd num]\n");
            printf("\t[-fg colour] [-bg colour] [-bw borderwidth]\n");

            exit(2);
        }
    }

    prev_focused = calloc(opt_vd, sizeof(Client *));
    if (!prev_focused) {
	    fprintf(stderr, "error at allocating prev_focused\n");
	    exit(1);
    }

    act.sa_handler = handle_signal;
    sigemptyset(&act.sa_mask);
#ifdef SA_NOCLDSTOP
    act.sa_flags = SA_NOCLDSTOP;  /* don't care about STOP, CONT */
#else
    act.sa_flags = 0;
#endif
    sigaction(SIGTERM, &act, NULL);
    sigaction(SIGINT, &act, NULL);
    sigaction(SIGHUP, &act, NULL);
    sigaction(SIGCHLD, &act, NULL);
    signal(SIGPIPE, SIG_IGN);

    setup_display();
    scan_windows();
    keys_init();

    throwAllUnmapEvent();
    throwUnmaps = 0;

    wm_running = 1;
    /* main event loop here */
    while (wm_running) {
        XNextEvent(dpy, &ev);
        switch (ev.type) {
        case KeyPress:
            handle_key_event(&ev.xkey); break;
        case ButtonPress:
            handle_button_event(&ev); break;
        case ButtonRelease:
            XSync(dpy, False);
            XAllowEvents(dpy, ReplayPointer, CurrentTime);
            XSync(dpy, False);
            break;
        case ConfigureRequest:
            handle_configure_request(&ev.xconfigurerequest); break;
        case MapRequest:
            handle_map_request(&ev.xmaprequest); break;
        case ClientMessage:
            handle_client_message(&ev.xclient); break;
        case EnterNotify:
            handle_enter_event(&ev.xcrossing); break;
        case PropertyNotify:
            handle_property_change(&ev.xproperty); break;
        case UnmapNotify:
            if (!throwUnmaps)
                handle_unmap_event(&ev.xunmap);
            break;
        case DestroyNotify:
            handle_destroy_event(&ev.xdestroywindow); break;
        }

	force_set_focus();
    }
    return 1;
}

