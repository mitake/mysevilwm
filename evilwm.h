#ifndef EVILWM_H_
#define EVILWM_H_

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>

#include "client.h"

/* default settings */

#define DEF_FONT	"lucidasans-10"
#define DEF_FG		"goldenrod"
#define DEF_BG		"grey50"
#define DEF_BW		1
#define DEF_FC		"blue"
#define SPACE		3
#define MINSIZE		15
#define DEF_VD		4

/* readability stuff */

#define NOT_QUITTING	0
#define QUITTING	1	/* for remove_client */
#define RAISE		1
#define NO_RAISE	0	/* for unhide() */

/* some coding shorthand */

#define ChildMask	(SubstructureRedirectMask|SubstructureNotifyMask)
#define ButtonMask	(ButtonPressMask|ButtonReleaseMask)
#define MouseMask	(ButtonMask|PointerMotionMask)

#define GRAB(w, mask, curs) \
	(XGrabPointer(dpy, w, False, mask, GrabModeAsync, GrabModeAsync, \
	None, curs, CurrentTime) == GrabSuccess)
#ifndef CLICK_FOCUS
#define setmouse(w, x, y) \
       XWarpPointer(dpy, None, w, 0, 0, 0, 0, x, y)
#endif
#define gravitate(c) \
	change_gravity(c, 1)
#define ungravitate(c) \
	change_gravity(c, -1)

/* declarations for global variables in main.c */

extern Display		*dpy;
extern Client		*current;
extern int		screen;
extern Window		root;
extern GC		invert_gc;
extern XFontStruct	*font;
extern Client		*head_client;
extern Atom		xa_wm_state;
extern Atom		xa_wm_change_state;
extern Atom		xa_wm_protos;
extern Atom		xa_wm_delete;
extern Atom		xa_wm_cmapwins;
extern Cursor		move_curs;
extern Cursor		resize_curs;
extern char		*opt_display;
extern const char *opt_font;
extern const char *opt_fg;
extern const char *opt_bg;
extern int		opt_vd;
extern int		opt_bw;
extern XColor		fg, bg, fc;
extern char		*opt_fc;
extern int		vdesk;
#ifdef SANITY
extern int		tracked_count;
#endif

extern char*		window_manager_name;

extern char null_str[];

extern char throwUnmaps;

extern int wm_running;

extern Client **prev_focused;

/* void do_event_loop(); */
int handle_xerror(Display *dpy, XErrorEvent *e);
int ignore_xerror(Display *dpy, XErrorEvent *e);
void dump_clients();
/*void spawn(const char *const cmd[]);*/
void spawn(char* cmd[]);
void handle_signal(int signo);
#ifdef DEBUG
void show_event(XEvent e);
#endif

#ifdef CLICK_FOCUS
void warp_pointer(Window w, int x, int y);
#endif

# define RD_DESK( desk ) ( (desk)->vdesk )

#ifdef DEBUG
#define LOG(msg, ...) do {                      \
        fprintf(stderr, msg,  ## __VA_ARGS__);  \
    } while (0)
#else
#define LOG(msg, ...) do {} while (0)
#endif

#endif // ! EVILWM_H_
