/* evilwm - Minimalist Window Manager for X
 * Copyright (C) 1999-2000 Ciaran Anscomb <ciarana@rd.bbc.co.uk>
 * see README for license and other details. */

#include "evilwm.h"
#include "misc.h"
#include "ctrl.h"
#include "str.h"
#include "sevilwm.h"

#include <X11/Xproto.h>
#include <stdlib.h>
#include <stdarg.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <fcntl.h>

void throwAllUnmapEvent() {
    XEvent ev;
    // throw all event
    while (XCheckTypedEvent(dpy, UnmapNotify, &ev)) {}
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

void handle_signal(int signo) {
    if (signo == SIGCHLD) {
        wait(NULL);
        return;
    }

    quit_nicely();
}

void cleanup() {
    char backup_file[MAXPATHLEN];

    mk_backfile_fullpath(backup_file);
    unlink(backup_file);

    while(head_client) remove_client(head_client, QUITTING);
    XSetInputFocus(dpy, PointerRoot, RevertToPointerRoot, CurrentTime);
    XInstallColormap(dpy, DefaultColormap(dpy, screen));
    XCloseDisplay(dpy);
}

int restart_argv_len = 0;
char * restart_argv = NULL;

void backup_argv(int argc, char ** argv) {
    int i, j;

    for(i = 1/* discard argv[0]*/; i < argc; i++) {
        restart_argv_len += strlen(argv[i]);
        restart_argv_len++;
    }
    restart_argv = calloc(restart_argv_len, sizeof(char));

    for(i = 1/* discard argv[0]*/, j = 0; i < argc; i++) {
        strcpy(restart_argv + j, argv[i]);
        j += strlen(argv[i]);
        restart_argv[j++] = ' ';
    }
    restart_argv[--j] = '\0';
}

void add_argv(int argc, char ** args, ctrl_message_callback message_cb) {
    if(argc != 2) {
        message_cb("ADDARGV must have one argument.\n");
        return;
    }

    if(restart_argv_len > 0) {
        restart_argv[restart_argv_len - 1] = ' ';
    }
    restart_argv = realloc(restart_argv,
                           restart_argv_len + strlen(args[1]) + 1);
    strcpy(restart_argv + restart_argv_len, args[1]);
    restart_argv_len += strlen(args[1]) + 1;
    restart_argv[restart_argv_len - 1] = '\0';
    printf("current restart_argv:%s, restart_argv_len:%d\n",
           restart_argv, restart_argv_len);
}

void get_argv(int argc, char ** args, ctrl_message_callback message_cb) {
}

void clear_argv(int argc, char ** args, ctrl_message_callback message_cb) {
    restart_argv_len = 0;
    free(restart_argv);
    restart_argv = NULL;
}

void backup_wins() {
    int fd;
    extern int errno;
    extern char * ipc_dir;
    char backup_name[MAXPATHLEN];
    Client* c;
    Str str = str_new(NULL);
    Window focused;
    int revert;

    bzero(backup_name, MAXPATHLEN);

    strcpy(backup_name, ipc_dir);
    backup_name[strlen(backup_name)] = '/';
    strcpy(backup_name + strlen(backup_name), WIN_BACK);

    unlink(backup_name);
    fd = open(backup_name, O_WRONLY | O_CREAT, S_IRUSR | S_IRUSR);
    if(fd < 0) {
        goto error;
    }

    XGetInputFocus(dpy, &focused, &revert);
    for (c = head_client; c; c = c->next) {
        str_cat(&str, "WIN(\"");
        str_cat_escaped(&str, c->name);
        str_catf(&str, 100,
                 "\", %d, %d, %d, %d, %d, %d, %d, '",
                 c->index, c->x, c->y, c->width, c->height, c->vdesk,
                 c->window == focused);
        if (c->mark) {
            char buf[2];
            buf[0] = c->mark;
            buf[1] = '\0';
            str_cat_escaped(&str, buf);
        }
        else {
            str_cat(&str, "\\0");
        }
        str_catf(&str, 100, "', 0x%x)\n", c->window);
    }
    write(fd, str.buf, str.size);
    str_free(&str);

  error:
    close(fd);
    return;
}

void restart() {
    char * args;

    if(!restart_argv) {
        execl("/bin/sh", "sh", "-c", window_manager_name, 0);
        return;
    }
    backup_wins();

    args = calloc(strlen(window_manager_name) + 1 /* space */
                  + strlen(restart_argv) + 1 /* NULL */
                  , sizeof(char));
    strcpy(args, window_manager_name);
    args[strlen(window_manager_name)] = ' ';
    strcpy(args + strlen(window_manager_name) + 1, restart_argv);
    args[strlen(args)] = '\0';
    execl("/bin/sh", "sh", "-c", args, 0);
}

void quit_nicely() {
    cleanup();
    exit(0);
}

int handle_xerror(Display *d, XErrorEvent *e) {
    Client *c = find_client(e->resourceid);

    /* if (e->error_code == BadAccess && e->resourceid == root) { */
    if (e->error_code == BadAccess &&
        e->request_code == X_ChangeWindowAttributes) {
#ifdef STDIO
        fprintf(stderr, "root window unavailable (maybe another wm is running?)\n");
#endif
        exit(1);
    }
    fprintf(stderr, "XError %x %d ", e->error_code, e->request_code);
    if (c && e->error_code != BadWindow) {
#ifdef XDEBUG
        fprintf(stderr, "(removing client)\n");
#endif
        remove_client(c, NOT_QUITTING);
    }
    return 0;
}

int ignore_xerror(Display *d, XErrorEvent *e) {
    return 0;
}

#ifdef DEBUG
void dump_clients() {
    Client *c;

    for (c = head_client; c; c = c->next)
        fprintf(stderr, "MISC: (%d) @ %d,%d\n", wm_state(c),
                c->x, c->y);
/*
    for (c = head_client; c; c = c->next)
        fprintf(stderr, "MISC: (%d, %d) @ %d,%d\n", wm_state(c),
                c->ignore_unmap, c->x, c->y);
*/
}
#endif /* DEBUG */

#ifdef SANITY
void sanity_check() {
    Client *c;
    int count = 0;

    for (c = head_client; c; c = c->next)
        count++;
    if (count != tracked_count) {
        fprintf(stderr, "misc:sanity_check() failed - should be %d clients, I count %d\n", tracked_count, count);
        exit(1);
    }
}
#endif /* SANITY */
