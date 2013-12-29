/* evilwm - Minimalist Window Manager for X
 * Copyright (C) 1999-2000 Ciaran Anscomb <ciarana@rd.bbc.co.uk>
 * see README for license and other details. */

#include "evilwm.h"
#include "newclient.h"
#include "screen.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

char null_str[] = "";

void make_new_client(Window w) {
    Client *c, *p;
    XWindowAttributes attr;
    long dummy;
    XWMHints *hints;
    XSizeHints* shints;

    XGrabServer(dpy);
    XSetErrorHandler(ignore_xerror);
    hints = XGetWMHints(dpy, w);

    shints = XAllocSizeHints();
    XGetWMNormalHints(dpy, w, shints, &dummy);

    c = (Client *)malloc(sizeof(Client));
    memset(c, 0, sizeof(*c));
    c->next = head_client;
    head_client = c;

    /* initialise(c, w); */
    c->window = w;
    c->name = null_str;

    /* try these here instead */
    c->size = shints;

    XClassHint* hint = XAllocClassHint();
    XGetClassHint(dpy, w, hint);
    if (hint->res_class) {
        c->name = hint->res_class;
        if (hint->res_name) XFree(hint->res_name);
    }
    XFree(hint);

    XGetWindowAttributes(dpy, c->window, &attr);

    c->x = attr.x;
    c->y = attr.y;
    c->width = attr.width;
    c->height = attr.height;
    c->border = opt_bw;
    c->oldw = c->oldh = 0;
    c->vdesk = vdesk;
    c->index = 0;

    if (!(attr.map_state == IsViewable)) {
        init_position(c);
        if (hints && hints->flags & StateHint)
            set_wm_state(c, hints->initial_state);
    }

    if (hints) XFree(hints);

    /* client is initialised */

    gravitate(c);
    reparent(c);

    XGrabButton(dpy, AnyButton, Mod1Mask, c->parent, False,
                ButtonMask, GrabModeSync, GrabModeAsync, None, None);
    XGrabButton(dpy, AnyButton, 0, c->parent, False,
                ButtonPressMask, GrabModeSync, GrabModeAsync, None, None);
    XMapWindow(dpy, c->window);
    XMapRaised(dpy, c->parent);
    set_wm_state(c, NormalState);

    XSync(dpy, False);
    XSetErrorHandler(handle_xerror);
    XUngrabServer(dpy);

    throwUnmaps = 0;

again:
    for (p = c->next; p; p = p->next) {
        if (c->index == p->index && !strcmp(c->name, p->name)) {
            c->index++;
            goto again;
        }
    }
}

void init_position(Client *c) {
    int x, y;
    int xmax = DisplayWidth(dpy, screen);
    int ymax = DisplayHeight(dpy, screen);

    if (c->size->flags & (/*PSize | */USSize)) {
        c->width = c->size->width;
        c->height = c->size->height;
    }

    if (c->width < MINSIZE) c->width = MINSIZE;
    if (c->height < MINSIZE) c->height = MINSIZE;
    if (c->width > xmax) c->width = xmax;
    if (c->height > ymax) c->height = ymax;

    if (c->size->flags & (/*PPosition | */USPosition)) {
        c->x = c->size->x;
        c->y = c->size->y;
        if (c->x < 0 || c->y < 0 || c->x > xmax || c->y > ymax)
            c->x = c->y = c->border;
    } else {
        get_mouse_position(&x, &y);
        c->x = (int) ((x / (float)xmax) * (xmax - c->border - c->width));
        c->y = (int) ((y / (float)ymax) * (ymax - c->border - c->height));
    }
}

void reparent(Client *c) {
    XSetWindowAttributes p_attr;

    XSelectInput(dpy, c->window, EnterWindowMask | PropertyChangeMask);

    p_attr.override_redirect = True;
    p_attr.background_pixel = bg.pixel;
    p_attr.event_mask = ChildMask | ButtonPressMask | ExposureMask | EnterWindowMask;
    c->parent = XCreateWindow(dpy, root, c->x-c->border, c->y-c->border,
                              c->width+(c->border*2), c->height + (c->border*2), 0,
                              DefaultDepth(dpy, screen), CopyFromParent,
                              DefaultVisual(dpy, screen),
                              CWOverrideRedirect | CWBackPixel | CWEventMask, &p_attr);

    XAddToSaveSet(dpy, c->window);
    XSetWindowBorderWidth(dpy, c->window, 0);
    /* Hmm, why resize this? */
    /* XResizeWindow(dpy, c->window, c->width, c->height); */
    XReparentWindow(dpy, c->window, c->parent, c->border, c->border);

    send_config(c);
}
