#include "evilwm.h"
#include "misc.h"
#include "ignore.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ADDIGN(x) x,
static char* ignore_orig[] = {
#include "config_ign.def"
};
#undef ADDIGN

char ** ignore;
int ignore_len;

void ignore_init(void) {
    int i;

    ignore_len = sizeof(ignore_orig) / sizeof(ignore_orig[0]);

    ignore = calloc(sizeof(char *), ignore_len);
    for(i = 0; i < ignore_len; i++) {
        char * newign;
        newign = calloc(sizeof(char), strlen(ignore_orig[i]) + 1);
        strcpy(newign, ignore_orig[i]);
        ignore[i] = newign;
    }
}

int is_ignore_impl(char * name) {
    int i;

    for(i = 0; i < ignore_len; i++) {
        if(!ignore[i]) {
            continue;
        }
        if(!strcmp(ignore[i], name)) return 1;
    }

    return 0;
}

int is_ignore(Client* c) {
    int i;
    for (i = 0; i < ignore_len; i++) {
        if (!ignore[i]) continue;
        if (strcmp(ignore[i], c->name) == 0) return 1;
    }
    return 0;
}

void add_ignore(int argc, char ** args, ctrl_message_callback message_cb) {
    int i;
    char * newign;

    if(argc != 2) {
        message_cb("ADDIGN must have one argument.\n");
        return;
    }

    if(is_ignore_impl(args[1])) {
        message_cb("Already in ignores list.\n");
        return;     /* already in ignores list */
    }

    for(i = 0; i < ignore_len; i++) {
        if(!ignore[i]) {
            newign = calloc(sizeof(char), strlen(args[1]) + 1);
            strcpy(newign, args[1]);
            ignore[i] = newign;
            message_cb(NULL);
            return;
        }
    }

    ignore = realloc(ignore, sizeof(char*) * ++ignore_len);
    newign = calloc(sizeof(char), strlen(args[1]) + 1);
    strcpy(newign, args[1]);
    ignore[ignore_len - 1] = newign;
    message_cb(NULL);
}

void del_ignore(int argc, char ** args, ctrl_message_callback message_cb) {
    int i;

    if(argc != 2) {
        message_cb("DELIGN must have one argument.\n");
        return;
    }

    for(i = 0; i < ignore_len; i++) {
        if(!ignore[i]) {
            continue;
        }
        if(!strcmp(args[1], ignore[i])) {
            free(ignore[i]);
            ignore[i] = NULL;
            message_cb(NULL);
            return;
        }
    }
    message_cb("Not in ignores list.\n");
}
