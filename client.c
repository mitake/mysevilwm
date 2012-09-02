/* evilwm - Minimalist Window Manager for X
 * Copyright (C) 1999-2000 Ciaran Anscomb <ciarana@rd.bbc.co.uk>
 * see README for license and other details. */

#include "evilwm.h"
#include "client.h"
#include "ignore.h"
#include "screen.h"

#include <stdlib.h>
#include <stdio.h>

static int send_xmessage(Window w, Atom a, long x);

/* used all over the place.  return the client that has specified window as
 * either window or parent */

Client *find_client(Window w) {
    Client *c;

    for (c = head_client; c; c = c->next)
        if (w == c->parent || w == c->window) return c;
    return NULL;
}

void set_wm_state(Client *c, int state) {
    long data[2];

    data[0] = (long) state;
    data[1] = None; /* icon window */

#ifdef DEBUG
    fprintf(stderr, "set_wm_state();\n");
#endif
    XChangeProperty(dpy, c->window, xa_wm_state, xa_wm_state,
                    32, PropModeReplace, (unsigned char *)data, 2);
}

int wm_state(Client *c) {
    Atom real_type;
    int real_format;
    unsigned long n, extra;
    long *data, state = WithdrawnState;

    if ((XGetWindowProperty(dpy, c->window, xa_wm_state, 0L, 2L, False,
                            AnyPropertyType, &real_type, &real_format, &n,
                            &extra, (unsigned char **)&data) == Success) && n) {
        state = *data;
        XFree(data);
    }
    return state;
}

void send_config(Client *c) {
    XConfigureEvent ce;

    ce.type = ConfigureNotify;
    ce.event = c->window;
    ce.window = c->window;
    ce.x = c->x;
    ce.y = c->y;
    ce.width = c->width;
    ce.height = c->height;
    ce.border_width = 0;
    ce.above = None;
    ce.override_redirect = 0;

    XSendEvent(dpy, c->window, False, StructureNotifyMask, (XEvent *)&ce);
}

void remove_client(Client *c, int from_cleanup) {
    Client *p =0;

    XGrabServer(dpy);
    XSetErrorHandler(ignore_xerror);

    if (c->name && c->name != null_str) XFree(c->name);

    if (from_cleanup) {
        unhide(c, NO_RAISE);
    }
    else if (!from_cleanup)
        set_wm_state(c, WithdrawnState);

    ungravitate(c);
    XReparentWindow(dpy, c->window, root, c->x, c->y);
    XRemoveFromSaveSet(dpy, c->window);
    XSetWindowBorderWidth(dpy, c->window, 1);

    XDestroyWindow(dpy, c->parent);

    if (head_client == c) head_client = c->next;
    else for (p = head_client; p && p->next; p = p->next)
        if (p->next == c) p->next = c->next;

    if (c->size) XFree(c->size);

    /* an enter event should set this up again */
    if (c == current) current = NULL;

    free(c);
#ifdef SANITY
    tracked_count--;
#endif
#ifdef DEBUG
    {
        Client *p;
        int i = 0;
        for (p = head_client; p; p = p->next)
            i++;
        fprintf(stderr, "CLIENT: free(), window count now %d\n", i);
    }
#endif

    XSync(dpy, False);
    XSetErrorHandler(handle_xerror);
    XUngrabServer(dpy);
}

void change_gravity(Client *c, int multiplier) {
    int dx = 0, dy = 0;
    int gravity = (c->size->flags & PWinGravity) ?
        c->size->win_gravity : NorthWestGravity;

    switch (gravity) {
    case NorthWestGravity:
    case SouthWestGravity:
    case NorthEastGravity:
        dx = c->border;
    case NorthGravity:
        dy = c->border; break;
        /* case CenterGravity: dy = c->border; break; */
    }

    c->x += multiplier * dx;
    c->y += multiplier * dy;
}

void send_wm_delete(Client *c) {
    int i, n, found = 0;
    Atom *protocols;

    if (c) {
        if (XGetWMProtocols(dpy, c->window, &protocols, &n)) {
            for (i=0; i<n; i++) if (protocols[i] == xa_wm_delete) found++;
            XFree(protocols);
        }
        if (found) send_xmessage(c->window, xa_wm_protos, xa_wm_delete);
        else XKillClient(dpy, c->window);
    }
}

static int send_xmessage(Window w, Atom a, long x) {
    XEvent ev;

    ev.type = ClientMessage;
    ev.xclient.window = w;
    ev.xclient.message_type = a;
    ev.xclient.format = 32;
    ev.xclient.data.l[0] = x;
    ev.xclient.data.l[1] = CurrentTime;

    return XSendEvent(dpy, w, False, NoEventMask, &ev);
}
/*
  void client_to_front(Client *c) {
  Client *p;

  if (head_client == c) {
  head_client = c->next;
  }
  for (p = head_client; p->next; p = p->next) {
  if (p->next == c) {
  p->next = c->next;
  }
  }
  p->next = c;
  c->next = NULL;
  }
*/

