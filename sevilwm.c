/* evilwm - Minimalist Window Manager for X
 * Copyright (C) 1999-2000 Ciaran Anscomb <ciarana@rd.bbc.co.uk>
 * see README for license and other details. */

#include "evilwm.h"
#include "sevilwm.h"
#include "events.h"
#include "newclient.h"
#include "misc.h"
#include "ipc.h"
#include "wins.h"
#include "screen.h"

#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <X11/cursorfont.h>
#include <stdio.h>

/* To change our modifiers ALT from CTRL+ALT, enable this macro.
#define SEVILWM_MODIFIER_ALT
*/
#ifdef SEVILWM_MODIFIER_ALT
const unsigned int sevilwm_key_event_modifiers = Mod1Mask;
#else
const unsigned int sevilwm_key_event_modifiers = ControlMask | Mod1Mask;
#endif

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
#ifdef SANITY
int     tracked_count = 0;
#endif
int wm_running;

char*           window_manager_name;

char throwUnmaps;

int orig_argc;
char** orig_argv;
extern int restarted;

int main(int argc, char *argv[]) {
    struct sigaction act;
    int i;
    XEvent ev;

    orig_argc = argc;
    orig_argv = argv;

    window_manager_name = argv[0];
    backup_argv(argc, argv);

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
    ipc_init_part();
    init_restart_configs();
    scan_windows();
    ipc_init();
    keys_init();
    ignore_init();

    throwAllUnmapEvent();
    throwUnmaps = 0;

    restarted = 0;
    wm_running = 1;
    /* main event loop here */
    while (wm_running) {
#ifdef SANITY
        sanity_check();
#endif
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

        if (!XPending(dpy)) {
            ipc_process();
        }
    }
    return 1;
}

void setup_display() {
    XGCValues gv;
    XSetWindowAttributes attr;
    XColor dummy;

#ifdef XDEBUG
    fprintf(stderr, "main:XOpenDisplay(); ");
#endif
    dpy = XOpenDisplay(opt_display);
    if (!dpy) { 
#ifdef STDIO
        fprintf(stderr, "can't open display %s\n", opt_display);
#endif
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
    extern int restart_focused_vdesk;
    extern Client * restart_focused_client;

#ifdef XDEBUG
    fprintf(stderr, "main:XQueryTree(); ");
#endif
    XQueryTree(dpy, root, &dw1, &dw2, &wins, &nwins);
#ifdef XDEBUG
    fprintf(stderr, "%d windows\n", nwins);
#endif
    for (i = 0; i < nwins; i++) {
        XGetWindowAttributes(dpy, wins[i], &attr);
        if (!attr.override_redirect && attr.map_state == IsViewable)
            make_new_client(wins[i]);
    }
    XFree(wins);

    if(restarted) {
	switch_vdesk(restart_focused_vdesk);
	focus(restart_focused_client);
    }
}
