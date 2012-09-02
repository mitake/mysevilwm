#include "ctrl.h"
#include "evilwm.h"
#include "str.h"
#include "parser.h"
#include "screen.h"
#include "events.h"
#include "ev_handler.h"
#include "keys.h"
#include "ignore.h"
#include "misc.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

static void * search_handler(char * name) {
    extern void * evhandler_array_start; /* in ev_handler.c */
    extern int evhandler_num;
    int i = 0;
    struct ev_handler * array = evhandler_array_start;

    for(i = 1; i <= evhandler_num; i++) {
        if(!strcmp(array[i].name, name)) {
            LOG("handler hit!:%s\n",array[i].name);
            LOG("address:0x%p\n",array[i].handler);
            return array[i].handler;
        }
    }

    return NULL;
}

#define SHIFT (1 << 24)
#define CTRL (1 << 25)
#define ALT (1 << 26)

static int parse_sac(char * keyseq) {
    /* fixme */
    int ret = 0;

    if(strstr(keyseq, "SHIFT")) {
        LOG("SHIFT found\n");
        ret |= SHIFT;
    }
    if(strstr(keyseq, "CTRL")) {
        LOG("CTRL found\n");
        ret |= CTRL;
    }
    if(strstr(keyseq, "ALT")) {
        LOG("ALT found\n");
        ret |= ALT;
    }

    LOG("OR ret:%x\n", ret);
    return ret;
}

static int search_keycode(char * keyname) {
    int mod, i;
    KeySym key;

    mod = parse_sac(keyname);
    keyname = strstr(keyname, "XK_");
    if (!keyname) {
        return -1;
    }
    for(i = 0; i < strlen(keyname); i++) {
        if(keyname[i] == '|' || keyname[i] == ' ') {
            keyname[i] = '\0';
            break;
        }
    }

    key = XStringToKeysym(keyname+3);
    if (key == NoSymbol) {
        return -1;
    }
    LOG("keycode found! code:%d name:%s\n", key, keyname);
    return key | mod;
}

static void key_conf_exec(int argc, char ** args,
                          ctrl_message_callback message_cb) {
    int i;
    int keycode = -1, keys_size;
    void (*handler)(EvArgs) = NULL;
    extern Key* keys;
    extern int key_len;
    char * newarg = NULL;
    Key* tmp_keys;

    if(argc != 4) {
        LOG("key_conf_exec():invalid format\n");
        message_cb("KEY takes 4 arguments\n");
        return;
    }
/*  LOG("args[1]:%s, args[2]:%s, args[3]:%s\n", */
/*      args[1], args[2], args[3]); */

    keycode = search_keycode(args[1]);
    if(keycode < 0) {
        LOG("invalid key name:%s\n", args[1]);
        message_cb("Invalid key name\n");
        return;
    }
    handler = search_handler(args[2]);
    if(!handler) {
        LOG("invalid handler name:%s\n", args[2]);
        message_cb("Invalid key handler name\n");
        return;
    }

    /* fixme memo:keys_args[keys_size - 1] = args[3]; */
    newarg = calloc(sizeof(char), strlen(args[3]) + 1);
    strcpy(newarg, args[3]);

    for (i = 0; i < key_len; i++) {
        if (keys[i].key == keycode) {
            keys[i].ev = handler;
            free(keys[i].arg);
            keys[i].arg = newarg;
            message_cb(NULL);
            return;
        }
    }

    tmp_keys = (Key*)realloc(keys, sizeof(Key) * ++key_len);
    if (!tmp_keys) {
        free(newarg);
        message_cb("Failed to alloc memory.\n");
        return;
    }
    keys = tmp_keys;
    keys[i].key = keycode;
    keys[i].ev = handler;
    keys[i].arg = newarg;
    message_cb(NULL);
}

void ctrl_exec(char* cmd, ctrl_message_callback message_cb) {
    char* args[11];
    int argc = parse_cmd(cmd, message_cb, args);
    if (argc <= 0) {
        message_cb("No commands.\n");
        return;
    }

#ifdef STDIO
    {
        int i;
        LOG("Run command: args=%s", args[0]);
        for (i = 1; i < argc; i++) {
            LOG(", %s", args[i]);
        }
        LOG("\n");
    }
#endif
    if (!strcmp(args[0], "SHUTDOWN")) {
        if (argc != 1) {
            message_cb("SHUTDOWN takes no argument\n");
            return;
        }
        wm_running = 0;
    }
    else if (!strcmp(args[0], "VDESK")) {
        int vd;
        if (argc != 2) {
            message_cb("VDESK takes an argument\n");
            return;
        }
        if (!isdigit(args[1][0])) {
            message_cb("Vdesk must be a positive integer\n");
            return;
        }
        vd = atoi(args[1]);
        if (vd >= opt_vd) {
            message_cb("Too large vdesk number\n");
            return;
        }
        switch_vdesk(vd);
    }
    else if (!strcmp(args[0], "WIN")) {
        Client* client;
        const char* name;
        int index;
        int x, y, w, h, vd, focused;
        char mark;
        if (argc != 10) {
            message_cb("WIN takes 10 arguments\n");
            return;
        }
        name = args[1];
        if (!isdigit(args[2][0])) {
            message_cb("Window index must be a positive integer\n");
            return;
        }
        index = atoi(args[2]);
        if (!isdigit(args[3][0])) {
            message_cb("Window X must be a positive integer\n");
            return;
        }
        x = atoi(args[3]);
        if (!isdigit(args[4][0])) {
            message_cb("Window Y must be a positive integer\n");
            return;
        }
        y = atoi(args[4]);
        if (!isdigit(args[5][0])) {
            message_cb("Window W must be a positive integer\n");
            return;
        }
        w = atoi(args[5]);
        if (!isdigit(args[6][0])) {
            message_cb("Window H must be a positive integer\n");
            return;
        }
        h = atoi(args[6]);
        if (!isdigit(args[7][0]) && strcmp(args[7], "-1")) {
            message_cb("Window vdesk must be a positive integer or -1\n");
            return;
        }
        vd = atoi(args[7]);
        if (!isdigit(args[8][0])) {
            message_cb("Window focused must be a positive integer\n");
            return;
        }
        focused = atoi(args[8]);
        mark = args[9][0] ? args[9][1] ? atoi(args[9]) : args[9][0] : 0;

        for (client = head_client; client; client = client->next) {
            if (client->index == index && !strcmp(client->name, name)) {
                break;
            }
        }
        if (!client) {
            message_cb("No such window\n");
            return;
        }

        change_state(client, x, y, w, h, vd, focused, mark);
    }
    else if(!strcmp(args[0], "KEY")) {
        key_conf_exec(argc, args, message_cb);
        return;
    }
    else if(!strcmp(args[0], "ADDIGN")) {
        add_ignore(argc, args, message_cb);
        return;
    }
    else if(!strcmp(args[0], "DELIGN")) {
        del_ignore(argc, args, message_cb);
        return;
    }
    else if(!strcmp(args[0], "ADDARGV")) {
	add_argv(argc, args, message_cb);
    }
    else if(!strcmp(args[0], "CLEARARGV")) {
	clear_argv(argc, args, message_cb);
    }
    else {
        message_cb("Unknown command\n");
        return;
    }
    XFlush(dpy);
    message_cb(NULL);
}

void ctrl_parse(char* cmds, int* parsed, ctrl_message_callback message_cb) {
    char* p;
    assert(cmds);
    assert(parsed);
    assert(*parsed >= 0);
    cmds += *parsed;

    for (p = cmds; *p; p++) {
        if (*p == '\n') {
            Str cmd = str_new(NULL);
            str_cat_buf(&cmd, cmds, p - cmds);
            *parsed += p - cmds + 1;
            ctrl_exec(cmd.buf, message_cb);
            cmds = p + 1;
            str_free(&cmd);
            LOG("parsed: %d\n", *parsed);
        }
    }
}
