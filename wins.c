#include "keys.h"
#include "client.h"
#include "screen.h"
#include "sevilwm.h"
#include "parser.h"

#include <string.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <fcntl.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#define WIN(NAME, INDEX, X, Y, W, H, VD, FOCUSES, MARK, WINID) \
    {                                                          \
        .name = NAME,                                          \
        .index = INDEX,                                        \
        .x = X,                                                \
        .y = Y,                                                \
        .width = W,                                            \
        .height = H,                                           \
        .vdesk= VD,                                            \
        .mark = MARK,                                          \
        .window = WINID                                        \
    },

static Client configs[] = {
#include "config_win.def"
    {
        .name = NULL,
    }
};

#undef WIN

static Client ** restart_configs;
static int restart_configs_num;
static Window restart_focused;
Client * restart_focused_client;
int restart_focused_vdesk = 1;
int restarted;

static void callback_msg(const char* msg) {
    fputs(msg, stdout);
}

void add_restart_configs(char * line) {
    int ret, i;
    char * args[10];

    bzero(&args[0], 10);
    ret = parse_cmd(line, callback_msg, args);
    if(ret < 0) {
        printf("add_restart_configs():failed at parse_cmd() ... %d\n", ret);
        return;
    }

    i = restart_configs_num;
    restart_configs = realloc(restart_configs, (++restart_configs_num) * sizeof(Client *));
    restart_configs[i] = calloc(1, sizeof(Client));

    restart_configs[i]->name = args[1];
    restart_configs[i]->index = atoi(args[2]);
    restart_configs[i]->x = atoi(args[3]);
    restart_configs[i]->y = atoi(args[4]);
    restart_configs[i]->width = atoi(args[5]);
    restart_configs[i]->height = atoi(args[6]);
    restart_configs[i]->vdesk = atoi(args[7]);
    restart_configs[i]->mark = *args[9];
    restart_configs[i]->window = strtol(args[10] + 2, NULL, 16);

    if(atoi(args[8])) {
        restart_focused = restart_configs[i]->window;
        restart_focused_vdesk = restart_configs[i]->vdesk;
    }
}

void init_restart_configs() {
    int fd, i = 0, rsize;
    char backup_name[MAXPATHLEN];
    char * line;
    char rbyte;

    fd = open(backup_name, O_RDONLY);
    if(fd < 0) {
        printf("error at %s\n", backup_name);
        return;
    }

    restarted = 1;
    line = calloc(BUFSIZ, sizeof(char));
    while(rsize = read(fd, &rbyte, 1)) {
        if(rsize < 0) {
            printf("error at reading %s\n", backup_name);
            break;
        } else if(rsize == 0) {
            break;		/* EOF */
        }

        if(rbyte == '\n') {
            add_restart_configs(line);
            i = 0;
            /* free(line); */ // unknown increment of pointer! FIXME
            line = calloc(BUFSIZ, sizeof(char));
            continue;
        }

        line[i++] = rbyte;
        assert(i != BUFSIZ);	/* FIXME? */
    }
    close(fd);
}

void apply_window_config(Client* cl) {
    int i, focused = 0;
    extern int opt_vd;

    if(!restarted)
        goto skip_restart;

    for(i = 0; i < restart_configs_num; i++) {
        Client * c = restart_configs[i];
        if(c->window == cl->window) {
            if(restart_focused == cl->window) {
                focused = 1;
                /* switch_vdesk(cl->vdesk); */
                restart_focused_client = cl;
            }
            if(c->vdesk > opt_vd) {
                c->vdesk = 1;
            }
            change_state(cl, c->x, c->y, c->width, c->height,
                         c->vdesk, focused, c->mark);
            return;
        }
    }

skip_restart:
    for (i = 0; configs[i].name; i++) {
        Client* c = &configs[i];
        if (c->index == cl->index && !strcmp(c->name, cl->name)) {
            /* We don't change focus in this function */
            change_state(cl, c->x, c->y, c->width, c->height,
                         c->vdesk, 1, c->mark);
            return;
        }
    }
}
