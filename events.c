/* evilwm - Minimalist Window Manager for X
 * Copyright (C) 1999-2000 Ciaran Anscomb <ciarana@rd.bbc.co.uk>
 * see README for license and other details. */

#include "evilwm.h"
#include "events.h"
#include "client.h"
#include "screen.h"
#include "newclient.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <X11/Xatom.h>
#include <X11/cursorfont.h>

Client* cl;

#ifdef CLICK_FOCUS
int driveEnterNotify = 1;
#endif

void handle_key_event(XKeyEvent *e) {
    cl = current;

    if (keys_run(e)) {
        /* Resend events which we didn't handle. */
        if (cl) {
            e->time = CurrentTime;
            e->window = cl->window;
            XSendEvent(dpy, cl->window, True, NoEventMask, (XEvent *)e);
            XSync(dpy, False);
        }
    }
}

void handle_button_event(XEvent *ev) {
    XButtonEvent* e = &ev->xbutton;

#ifdef CLICK_FOCUS
    /* ad-hoc addition for click and focus model */
    Client *target = find_client(e->window);
    if (target) set_focus(target);
#endif

    cl = current;

    keys_run_button(e);

    XSync(dpy, False);
    XAllowEvents(dpy, ReplayPointer, CurrentTime);
    XSync(dpy, False);
}

void handle_configure_request(XConfigureRequestEvent *e) {
    Client *c = find_client(e->window);
    XWindowChanges wc;

    wc.sibling = e->above;
    wc.stack_mode = e->detail;
    if (c) {
        ungravitate(c);
        if (e->value_mask & CWX) c->x = e->x;
        if (e->value_mask & CWY) c->y = e->y;
        if (e->value_mask & CWWidth) c->width = e->width;
        if (e->value_mask & CWHeight) c->height = e->height;
        gravitate(c);

        wc.x = c->x - c->border;
        wc.y = c->y - c->border;
        wc.width = c->width + (c->border*2);
        wc.height = c->height + (c->border*2);
        wc.border_width = 0;
        XConfigureWindow(dpy, c->parent, e->value_mask, &wc);
        send_config(c);
    }

    wc.x = c ? c->border : e->x;
    wc.y = c ? c->border : e->y;
    wc.width = e->width;
    wc.height = e->height;
    XConfigureWindow(dpy, e->window, e->value_mask, &wc);
}

void handle_map_request(XMapRequestEvent *e) {
    Client *c = find_client(e->window);

#ifdef CLICK_FOCUS
    driveEnterNotify = 1;
#endif

    if (c) {
        if (c->vdesk == vdesk) unhide(c, RAISE);
    } else {
#ifdef DEBUG
        fprintf(stderr, "event:map->make_new_client(); ");
#endif
        make_new_client(e->window);
    }
}

void handle_unmap_event(XUnmapEvent* e) {
    Client* c = find_client(e->window);

#ifdef CLICK_FOCUS
    driveEnterNotify = 1;
#endif

    if (c) hide(c);
}

void handle_destroy_event(XDestroyWindowEvent *e) {
    Client *c = find_client(e->window);

    if (c) {
        remove_client(c, NOT_QUITTING);
    }
}

void handle_client_message(XClientMessageEvent *e) {
    Client *c = find_client(e->window);

    if (c && e->message_type == xa_wm_change_state &&
        e->format == 32 && e->data.l[0] == IconicState)
    {
        hide(c);
    }
}

void handle_property_change(XPropertyEvent *e) {
    Client *c = find_client(e->window);
    long dummy;

    if (c) {
        if (e->atom == XA_WM_NORMAL_HINTS)
            XGetWMNormalHints(dpy, c->window, c->size, &dummy);
    }
}

void set_focus(Client* c) {
    if (c->vdesk != vdesk && c->vdesk != -1)
        return;

    XSetInputFocus(dpy, c->window, RevertToPointerRoot, CurrentTime);
    if (current && c!=current) {
        XSetWindowBackground(dpy, current->parent, bg.pixel);
        XClearWindow(dpy, current->parent);
    }
    XSetWindowBackground(dpy, c->parent, (c->vdesk==-1)?fc.pixel:fg.pixel);
    XClearWindow(dpy, c->parent);
    current = c;

    if (0 < c->vdesk)
	    prev_focused[c->vdesk] = c;
}

void handle_enter_event(XCrossingEvent *e) {
    Client *c = find_client(e->window);
    if (c && current != c) {
#if 0
        /* IIRC, it was a fix for old skkinput. Since this works for
         * other application poorly, comment out for now.
         */
        XWMHints *hints = XGetWMHints(dpy, c->window);
        if (hints) {
            if ( (hints->flags&InputHint) && (hints->input==False) ) {
                XFree(hints);
                return;
            }
            XFree(hints);
        }
#endif

        if (e->type == EnterNotify
#ifdef CLICK_FOCUS
            && driveEnterNotify
#endif
            ) {
            set_focus(c);
#ifdef CLICK_FOCUS
            driveEnterNotify = 0;
#endif
        }
    }
}

void handle_focus_change_event(XFocusChangeEvent *e) {
    Client *c = find_client(e->window);
    if (c) {
        set_focus(c);
    }
}

void do_move() {
    move(cl, 1);
#ifdef CLICK_FOCUS
    warp_pointer(cl->window, cl->width + cl->border - 1,
                 cl->height + cl->border - 1);
    driveEnterNotify = 0;
#else
    setmouse(cl->window, cl->width + cl->border - 1,
             cl->height + cl->border - 1);
#endif
}

void ev_move_window_x(EvArgs args) {
    if (cl != 0) {
        cl->x += atoi(args);
        do_move();
    }
}

void ev_move_window_y(EvArgs args) {
    if (cl != 0) {
        cl->y += atoi(args);
        do_move();
    }
}

void do_resize() {
    resize(cl, 1);
#ifdef CLICK_FOCUS
    warp_pointer(cl->window, cl->width + cl->border - 1,
                 cl->height + cl->border - 1);
    driveEnterNotify = 0;
#else
    setmouse(cl->window, cl->width + cl->border - 1,
             cl->height + cl->border - 1);
#endif
}

void ev_resize_window_x(EvArgs args) {
    if (cl != 0) {
        if((cl->width + atoi(args)) < 0) {
            return;
        }
        cl->width += atoi(args);
        do_resize();
    }
}

void ev_resize_window_y(EvArgs args) {
    if (cl != 0) {
        if((cl->height + atoi(args)) < 0) {
            return;
        }
        cl->height += atoi(args);
        do_resize();
    }
}

void ev_move_window_upleft(EvArgs args) {
    if (cl != 0) {
        cl->x = cl->border;
        cl->y = cl->border;
        do_move();
    }
}

void ev_move_window_upright(EvArgs args) {
    if (cl != 0) {
        cl->x = DisplayWidth(dpy, screen) - cl->width - cl->border;
        cl->y = cl->border;
        do_move();
    }
}

void ev_move_window_downleft(EvArgs args) {
    if (cl != 0) {
        cl->x = cl->border;
        cl->y = DisplayHeight(dpy, screen)
            - cl->height - cl->border;
        do_move();
    }
}

void ev_move_window_downright(EvArgs args) {
    if (cl != 0) {
        cl->x = DisplayWidth(dpy, screen) - cl->width - cl->border;
        cl->y = DisplayHeight(dpy, screen)
            - cl->height - cl->border;
        do_move();
    }
}

void ev_delete_window(EvArgs args) {
    if (cl != 0) {
        send_wm_delete(cl);
    }
}

void ev_info_window(EvArgs args) {
    if (cl != 0) {
        show_info(cl);
    }
}

void ev_maximise_window(EvArgs args) {
    if (cl != 0) {
        maximise_horiz(cl);
        maximise_vert(cl);
        resize(cl, 1);
#ifdef CLICK_FOCUS
        warp_pointer(cl->window, cl->width + cl->border - 1,
                     cl->height + cl->border - 1);
        driveEnterNotify = 0;
#else
        setmouse(cl->window, cl->width + cl->border - 1,
                 cl->height + cl->border - 1);
#endif
    }
}

void ev_maximise_window_vert(EvArgs args) {
    if (cl != 0) {
        maximise_vert(cl);
        resize(cl, 1);
#ifdef CLICK_FOCUS
        warp_pointer(cl->window, cl->width + cl->border - 1,
                     cl->height + cl->border - 1);
#else
        setmouse(cl->window, cl->width + cl->border - 1,
                 cl->height + cl->border - 1);
#endif
    }
}

void ev_fix_window(EvArgs args) {
    if (cl != 0) {
        if (cl->vdesk == vdesk || cl->vdesk == -1) {
            XSetWindowBackground(dpy, cl->parent,
                                 cl->vdesk == -1 ? fg.pixel : fc.pixel);
            XClearWindow(dpy, cl->parent);
            cl->vdesk = cl->vdesk == -1 ? vdesk : -1;
        }
    }
}

void spawn(char* cmd[]) {
    pid_t pid;

#if 0
    int i;
    for (i = 0; i < 100; i++) {
        if (cmd[i] == NULL) break;
        printf("%s ", cmd[i]);
    }
    printf("\n");
#endif

/* @@@
   if (current_screen && current_screen->display)
   putenv(current_screen->display);
*/
    pid = fork();

    if (pid == 0) {
        execvp(cmd[0], (char *const *)&cmd[0]);
        printf("%s: error exec\n", cmd[0]);
        exit(0);
    }

    /* @@@ it's not work. why? */
#if 0
    if (!(pid = fork())) {
        setsid();
        switch (fork()) {
            /* explicitly hack around broken SUS execvp prototype */
        case 0: execvp(cmd[0], (char *const *)&cmd[1]);
        default: _exit(0);
        }
    }
    if (pid > 0)
        wait(NULL);
#endif
}

void ev_exec_command(EvArgs args) {
    char* a[64];
    char* tmp = (char*)malloc(strlen(args)+1);
    int i = 0;
    strcpy(tmp, args);
    a[i++] = strtok(tmp, " ");
    while ((a[i++] = strtok(NULL, " ")) != NULL) {}
    spawn(a);
    free(tmp);
}

void ev_next_focus(EvArgs args) {
    next(cl);
}

void ev_switch_vdesk(EvArgs args) {
#ifdef CLICK_FOCUS
    driveEnterNotify = 0;
#endif
    switch_vdesk(atoi(args));
}

void ev_next_vdesk(EvArgs args) {
#ifdef CLICK_FOCUS
    driveEnterNotify = 0;
#endif
    // XK_? is not good idea for vdesk number, I think.
    if (vdesk < opt_vd) switch_vdesk(vdesk + 1);
    else switch_vdesk(1);
}

void ev_prev_vdesk(EvArgs args) {
#ifdef CLICK_FOCUS
    driveEnterNotify = 0;
#endif
    if (vdesk > 1) switch_vdesk(vdesk - 1);
    else switch_vdesk(opt_vd);
}

void ev_fix_next_vdesk(EvArgs args) {
    ev_fix_window(args);
    ev_next_vdesk(args);
    ev_fix_window(args);
}

void ev_fix_prev_vdesk(EvArgs args) {
    ev_fix_window(args);
    ev_prev_vdesk(args);
    ev_fix_window(args);
}

void ev_up_focus(EvArgs args) {
    arrow_next(cl, 0, -1);
}

void ev_down_focus(EvArgs args) {
    arrow_next(cl, 0, 1);
}

void ev_right_focus(EvArgs args) {
    arrow_next(cl, 1, 0);
}

void ev_left_focus(EvArgs args) {
    arrow_next(cl, -1, 0);
}

void ev_upleft_focus(EvArgs args) {
    arrow_next(cl, -1, -1);
}

void ev_upright_focus(EvArgs args) {
    arrow_next(cl, 1, -1);
}

void ev_downleft_focus(EvArgs args) {
    arrow_next(cl, -1, 1);
}

void ev_downright_focus(EvArgs args) {
    arrow_next(cl, 1, 1);
}

void ev_wm_quit(EvArgs args) {
    quit_nicely();
}

void warp_pointer(
#ifdef CLICK_FOCUS
    Window w,
#endif
    int x, int y) {
#ifdef CLICK_FOCUS
    driveEnterNotify = 1;
    XWarpPointer(dpy, None, w, 0, 0, 0, 0, x, y);
#else
    XWarpPointer(dpy, None, None, 0, 0, 0, 0, x, y);
#endif
}

void ev_warp_pointer_x(EvArgs args) {
#ifdef CLICK_FOCUS
    warp_pointer(None, atoi(args), 0);
#else
    warp_pointer(atoi(args), 0);
#endif
}

void ev_warp_pointer_y(EvArgs args) {
#ifdef CLICK_FOCUS
    warp_pointer(None, 0, atoi(args));
#else
    warp_pointer(0, atoi(args));
#endif
}

void ev_set_mode(EvArgs args) {
/* @@@
    keys.setMode(atoi(str));
*/
}

void ev_raise_window(EvArgs args) {
    if (cl) XRaiseWindow(dpy, cl->parent);
}

void ev_lower_window(EvArgs args) {
    if (cl) XLowerWindow(dpy, cl->parent);
}

/*
void ev_iconize(EvArgs args) {
    if (cl) iconize(cl);
}
*/

void ev_mouse_move(EvArgs args) {
    if (cl) move(cl, 0);
}

void ev_mouse_resize(EvArgs args) {
    if (cl) resize(cl, 0);
}

void ev_do_nothing(EvArgs args) {
}
