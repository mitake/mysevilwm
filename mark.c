/* evilwm - Minimalist Window Manager for X
 * Copyright (C) 1999-2002 Ciaran Anscomb <evilwm@6809.org.uk>
 * see README for license and other details. */

#include "evilwm.h"
#include "mark.h"

#include <stdio.h>
#include <ctype.h>

static char prev_client_mark = '\0';    /* last client we were on before goto_mark was issued */

void mark_client(Client *c) {
    XEvent ev;
    char mark;
    int numchar;
    Client *oldc;

    if (XGrabKeyboard(dpy, root, False, GrabModeAsync, GrabModeAsync, CurrentTime) != GrabSuccess)
        return;

    /* wait for release KEY_MARK */
    XMaskEvent(dpy, KeyReleaseMask, &ev);

    /* get mark char */
    XMaskEvent(dpy, KeyPressMask, &ev);
    numchar = XLookupString(&ev.xkey, &mark, 1, NULL, NULL);
    if (numchar != 1)
        goto finish;
    
    if (!islower(mark)) /* in accord with vi */
        goto finish;
    
    if (!c)         /* not on any client */
        goto finish;

    /* find a client which has this 'mark' */
    oldc = find_marked_client(mark);
    if (oldc) {
        if (mark == prev_client_mark) {
            /* this client is prev_client; remember it */
            oldc->mark = '\'';
            prev_client_mark = '\'';
        } else
            oldc->mark = '\0';
    }
    
    /* mark current client with this 'mark' */
    c->mark = mark;

finish:
    XUngrabKeyboard(dpy, CurrentTime);
    return;
}

void goto_mark(Client *c) {
    XEvent ev;
    char mark;
    int numchar;
    Client *newc;

    if (XGrabKeyboard(dpy, root, False, GrabModeAsync, GrabModeAsync, CurrentTime) != GrabSuccess)
        return;

    /* wait for release KEY_GOTOMARK */
    XMaskEvent(dpy, KeyReleaseMask, &ev);

    /* get mark char */
    XMaskEvent(dpy, KeyPressMask, &ev);
    numchar = XLookupString(&ev.xkey, &mark, 1, NULL, NULL);
    if (numchar != 1)
        goto finish;

    if (mark == MARKCHAR_PREVCLIENT) {  /* then try to go back where we were */
        if (prev_client_mark == '\0')   /* no goto_mark has been issued so far */
            goto finish;
        else
            mark = prev_client_mark;    /* a-z or '\'' */
    } else if (!islower(mark))  /* in accord with vi */
        goto finish;
    
    /* find a client which has this 'mark' */
    newc = find_marked_client(mark);
    if (!newc)
        /* no client is with that mark */
        goto finish;
    
    /* if the destination was marked with '\'', clear it */
    if (newc->mark == '\'')
        newc->mark = '\0';

    /* remember the client we are leaving */
    if (c) {
        if (c->mark == '\0')
            c->mark = '\'';     /* mark this anonymous client with '\'' */
        prev_client_mark = c->mark;
    }

    switch_vdesk(RD_DESK(newc));

    unhide(newc, RAISE);
#ifdef CLICK_FOCUS
    warp_pointer(newc->window, newc->width + newc->border - 1,
                 newc->height + newc->border - 1);
#else
    setmouse(newc->window, newc->width + newc->border - 1,
             newc->height + newc->border - 1);
#endif

finish:
    XUngrabKeyboard(dpy, CurrentTime);
    return;
}

Client *find_marked_client(char mark) {
    Client *c;

    for (c = head_client; c; c = c->next)
        if (c->mark == mark) return c;
    return NULL;
}
