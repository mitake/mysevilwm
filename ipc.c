#include "evilwm.h"
#include "str.h"
#include "ctrl.h"
#include "events.h"
#include "keys.h"
#include "ignore.h"
#include "ev_handler.h"
#include "sevilwm.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <assert.h>
#include <errno.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/select.h>

extern char* config_key_def;
extern char* config_win_def;
extern char* config_ign_def;

enum {
    SOCK_X = 0,
    SOCK_CTRL,
    SOCK_CONFIG_KEY_ORIG,
    SOCK_CONFIG_WIN_ORIG,
    SOCK_CONFIG_IGN_ORIG,
    SOCK_WINDOWS,
    SOCK_KEYS,
    SOCK_IGNORES,
    SOCK_INFO,
    SOCK_ARGV,
    MAX_FDS
};
static int fds[MAX_FDS];
static int num_fds = 0;

typedef struct {
    int fd;
    int wfd;
    clock_t updated;
} inflight;
static inflight inflights[MAX_FDS];
static int num_inflights = 0;

char* ipc_dir;

static Str ctrl_str;
static int ctrl_parsed;

#ifdef STDIO
#define errprint(msg, ...) do {                                   \
        fprintf(stderr, msg,  ## __VA_ARGS__);                    \
        fprintf(stderr, "errno=%d:%s\n", errno, strerror(errno)); \
    } while (0)
#else
#define errprint(msg, ...) do {} while (0)
#endif

#define FREE(p) do {                            \
        free(p);                                \
        p = NULL;                               \
    } while (0)

static char* mstrcat(const char* base, const char* addend) {
    char* p = (char*)malloc(strlen(base) + strlen(addend) + 1);
    strcpy(p, base);
    strcat(p, addend);
    return p;
}

static char* join_path(const char* base, const char* path) {
    char* p = (char*)malloc(strlen(base) + strlen(path) + 2);
    strcpy(p, base);
    strcat(p, "/");
    strcat(p, path);
    return p;
}

static int remove_dir(const char* dirname) {
    struct dirent* ent;
    int err = 0;
    char* file = NULL;
    DIR* dir = opendir(dirname);
    if (dir == NULL) {
        return 0;
    }
    while ((ent = readdir(dir)) != NULL) {
        if (!strcmp(ent->d_name, ".") ||
            !strcmp(ent->d_name, "..") ||
	    !strcmp(ent->d_name, WIN_BACK)) {
            continue;
        }
        file = join_path(dirname, ent->d_name);
        err = unlink(file);
        if (err < 0) {
            errprint("failed to unlink: %s\n", file);
            goto error;
        }
        FREE(file);
    }
    return closedir(dir);
    /* return rmdir(dirname); */
error:
    FREE(file);
    closedir(dir);
    return err;
}

static void init_inflight(int fdindex) {
    assert(fdindex >= 0);
    memset(&inflights[fdindex], 0, sizeof(inflights[fdindex]));
}

static void push_fd(int fd) {
    LOG("push_fd: %d\n", fd);
    assert(num_fds < MAX_FDS);
    fds[num_fds] = fd;
    init_inflight(num_fds);
    num_fds++;
}

static int push_unix_socket(int fdindex, const char* filename) {
    int err = 0;
    int sock;
    struct sockaddr_un addr;

    assert(fdindex == num_fds);

    sock = socket(PF_UNIX, SOCK_STREAM, 0);
    if (sock < 0) {
        errprint("socket\n");
        goto error;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = PF_UNIX;
    const int unix_path_max = sizeof(addr.sun_path);
    err = snprintf(addr.sun_path, unix_path_max, "%s/%s", ipc_dir, filename);
    if (err < 0 || err >= unix_path_max) {
        errprint("Too long path: %s/%s\n", ipc_dir, filename);
        err = -1;
        goto error;
    }

    err = bind(sock, (struct sockaddr*)&addr, sizeof(addr));
    if (err < 0) {
        errprint("bind: %s\n", addr.sun_path);
        goto error;
    }

    err = listen(sock, 1);
    if (err < 0) {
        errprint("listen: %s\n", addr.sun_path);
        goto error;
    }

    push_fd(sock);

    return 0;
error:
    if (sock > 0) {
        close(sock);
    }
    return -1;
}

void ipc_init_part() {
    ipc_dir = mstrcat("/tmp/sevilwm_", DisplayString(dpy));
}

void ipc_init() {
    int err = 0;
    extern int errno;

    assert(dpy);
    push_fd(ConnectionNumber(dpy));

    /* ipc_dir = mstrcat("/tmp/sevilwm_", DisplayString(dpy)); <- now in ipc_init_part() */

    /* TODO: Removing directory may be overkill. */
    err = remove_dir(ipc_dir);
    if (err < 0) {
        goto error;
    }

    err = mkdir(ipc_dir, S_IRWXU);
    if((err < 0) && (errno != EEXIST)) {
        errprint("error at making directory:%s\n", ipc_dir);
        goto error;
    }

    err = push_unix_socket(SOCK_CTRL, "ctrl");
    if (err < 0) {
        goto error;
    }

    err = push_unix_socket(SOCK_CONFIG_KEY_ORIG, "config_key.def");
    if (err < 0) {
        goto error;
    }

    err = push_unix_socket(SOCK_CONFIG_WIN_ORIG, "config_win.def");
    if (err < 0) {
        goto error;
    }

    err = push_unix_socket(SOCK_CONFIG_IGN_ORIG, "config_ign.def");
    if (err < 0) {
        goto error;
    }

    err = push_unix_socket(SOCK_WINDOWS, "windows");
    if (err < 0) {
        goto error;
    }

    err = push_unix_socket(SOCK_KEYS, "keys");
    if (err < 0) {
        goto error;
    }

    err = push_unix_socket(SOCK_IGNORES, "ignores");
    if (err < 0) {
        goto error;
    }

    err = push_unix_socket(SOCK_INFO, "info");
    if (err < 0) {
        goto error;
    }

    err = push_unix_socket(SOCK_ARGV, "argv");
    if (err < 0) {
        goto error;
    }

    return;
error:
    ;  /* do nothing for now */
}

static int max(int x, int y) {
    return x > y ? x : y;
}

static void finish_unix_socket(int fdindex) {
    assert(fdindex < num_fds);
    assert(fdindex > 0);
    close(fds[fdindex]);
    fds[fdindex] = inflights[fdindex].fd;
    init_inflight(fdindex);
}

static void write_unix_socket(int fdindex, const char* buf, int len) {
    assert(fdindex < num_fds);
    assert(fdindex > 0);
    /* TODO: This can block WM */
    while (len) {
        int err = write(fds[fdindex], buf, len);
        if (err < 0) {
            errprint("write failed\n");
            // It's OK. The client closed without reading responses.
            return;
        }
        len -= err;
        buf += err;
    }
}

static void init_config_orig(int sock, const char* config_def) {
    write_unix_socket(sock, config_def, strlen(config_def));
    finish_unix_socket(sock);
}

void str_cat_escaped(Str* str, const char* addend) {
    const char* p;
    for (p = addend; *p; p++) {
        if (isprint(*p) && *p != '\'' && *p != '"' && *p != '\\') {
            str_catf(str, 2, "%c", *p);
        }
        else {
            str_catf(str, 5, "\\x%x", *p);
        }
    }
}

static void init_windows() {
    Client* c;
    Str str = str_new(NULL);
    Window focused;
    int revert;
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
    LOG("write windows size=%d\n", str.size);
    write_unix_socket(SOCK_WINDOWS, str.buf, str.size);
    str_free(&str);
    finish_unix_socket(SOCK_WINDOWS);
}

static const char * search_handler_string(void * handler) {
    extern void * evhandler_array_start; /* in ev_handler.c */
    extern int evhandler_num;
    int i = 0;
    struct ev_handler * array = evhandler_array_start;

    for(i = 1; i <= evhandler_num; i++) {
        if(array[i].handler == handler) {
            LOG("handler hit!:%s\n",array[i].name);
            LOG("address:0x%p\n",array[i].handler);
            return array[i].name;
        }
    }

    return NULL;
}

static void init_keys() {
    int i;
    extern Key* keys;
    extern int key_len;
    Str str = str_new(NULL);
    for (i = 0; i < key_len; i++) {
        if (keys[i].key == -1) continue;
        int key = keys[i].key;
        int mod = key >> 24;
        int sym = key & ((1 << 24) - 1);
        const char* key_str;
        const char* handler_str;
        str_cat(&str, "KEY(");
        key_str = XKeysymToString(sym);
        if (key_str) {
            str_cat(&str, "XK_");
            str_cat(&str, key_str);
        }
        else {
            str_catf(&str, 20, "%d", sym);
        }
        if (mod & 1) {
            str_cat(&str, "|SHIFT");
        }
        if (mod & 2) {
            str_cat(&str, "|CTRL");
        }
        if (mod & 4) {
            str_cat(&str, "|ALT");
        }
        str_cat(&str, ", ");
        handler_str = search_handler_string(keys[i].ev);
        if (handler_str) {
            str_cat(&str, handler_str);
        }
        else {
            str_cat(&str, "ev_do_nothing /* ERROR */");
        }
        str_cat(&str, ", \"");
        str_cat(&str, keys[i].arg);
        str_cat(&str, "\")\n");
    }
    LOG("write keys size=%d\n", str.size);
    write_unix_socket(SOCK_KEYS, str.buf, str.size);
    str_free(&str);
    finish_unix_socket(SOCK_KEYS);
}

static void init_ignores() {
    extern char** ignore;
    extern int ignore_len;
    int i;
    Str str = str_new(NULL);
    for (i = 0; i < ignore_len; i++) {
        if (!ignore[i]) continue;
        str_catf(&str, 20 + strlen(ignore[i]), "ADDIGN(\"%s\")\n", ignore[i]);
    }
    LOG("write ignores size=%d\n", str.size);
    write_unix_socket(SOCK_IGNORES, str.buf, str.size);
    str_free(&str);
    finish_unix_socket(SOCK_IGNORES);
}

static void init_info() {
    Client* c;
    Window focused;
    int revert;
    XGetInputFocus(dpy, &focused, &revert);
    Str str = str_new(window_manager_name);
    str_catf(&str, 30, " x=%d y=%d",
             DisplayWidth(dpy, screen), DisplayHeight(dpy, screen));
    str_catf(&str, 20, " vdesk=%d\n", vdesk);
    for (c = head_client; c; c = c->next) {
        int x = c->x;
        int y = c->y;
        int w = c->width;
        int h = c->height;
        if (c->oldw) {
            /* maximized */
            x = c->oldx;
            y = c->oldy;
            w = c->oldw;
            h = c->oldh;
        }
        str_catf(&str, strlen(c->name) + 100,
                 "%s:%d %dx%d+%d+%d vdesk=%d",
                 c->name, c->index, x, y, w, h, c->vdesk);
        if (c->window == focused) {
            str_cat(&str, " FOCUSED");
        }
        if (c->oldw) {
            str_cat(&str, " MAXIMIZED");
        }
        if (c->mark) {
            str_catf(&str, 20, " mark='%c'", c->mark);
        }
        str_cat(&str, "\n");
    }
    LOG("write info size=%d\n", str.size);
    write_unix_socket(SOCK_INFO, str.buf, str.size);
    str_free(&str);
    finish_unix_socket(SOCK_INFO);
}

static void init_ctrl() {
    ctrl_str = str_new("");
    ctrl_parsed = 0;
}

static void init_argv() {
    extern char * restart_argv;
    extern int restart_argv_len;

    write_unix_socket(SOCK_ARGV, restart_argv, restart_argv_len);
    finish_unix_socket(SOCK_ARGV);
}

static void handle_ctrl_msg(const char* msg) {
    if (msg) {
        write_unix_socket(SOCK_CTRL, msg, strlen(msg));
    }
    else {
        write_unix_socket(SOCK_CTRL, "ok\n", 3);
    }
}

static void handle_ctrl() {
    char buf[256];
    int len;
    len = read(fds[SOCK_CTRL], buf, sizeof(buf));
    if (len < 1) {
        int left = ctrl_str.size - ctrl_parsed;
        assert(left >= 0);
        if (left > 0) {
            str_free(&ctrl_str);
            str_catf(&ctrl_str, 50, "%d bytes of data left\n", left);
            write_unix_socket(SOCK_CTRL, ctrl_str.buf, ctrl_str.size);
        }
        str_free(&ctrl_str);

        LOG("ctrl finished\n");
        if (len < 0) {
            errprint("read\n");
        }
        finish_unix_socket(SOCK_CTRL);
        return;
    }
    str_cat_buf(&ctrl_str, buf, len);
    ctrl_parse(ctrl_str.buf, &ctrl_parsed, &handle_ctrl_msg);
}

static void read_unix_socket(int fdindex) {
    assert(fdindex < num_fds);
    assert(fdindex > 0);
    if (inflights[fdindex].fd) {
        switch (fdindex) {
        case SOCK_CTRL:
            handle_ctrl();
            break;

        default:
            errprint("Unexpected inflight sockfd: %d\n", fdindex);
            assert(0);
        }
    }
    else {
        struct sockaddr addr;
        int len = sizeof(addr);
        int fd = accept(fds[fdindex], &addr, &len);
        if (fd < 0) {
            errprint("accept\n");
            return;
        }
        inflights[fdindex].fd = fds[fdindex];
        inflights[fdindex].updated = clock();
        fds[fdindex] = fd;
        switch (fdindex) {
        case SOCK_CONFIG_KEY_ORIG:
            init_config_orig(SOCK_CONFIG_KEY_ORIG, config_key_def);
            break;

        case SOCK_CONFIG_WIN_ORIG:
            init_config_orig(SOCK_CONFIG_WIN_ORIG, config_win_def);
            break;

        case SOCK_CONFIG_IGN_ORIG:
            init_config_orig(SOCK_CONFIG_IGN_ORIG, config_ign_def);
            break;

        case SOCK_WINDOWS:
            init_windows();
            break;

        case SOCK_KEYS:
            init_keys();
            break;

        case SOCK_IGNORES:
            init_ignores();
            break;

        case SOCK_INFO:
            init_info();
            break;

        case SOCK_CTRL:
            init_ctrl();
            break;

        case SOCK_ARGV:
            init_argv();
            break;

        default:
            errprint("Unhandled sockfd: %d\n", fdindex);
            assert(0);
        }
    }
}

void ipc_process() {
    /* LOG("ipc_process()\n"); */
    while (wm_running) {
        int i;
        int err;
        int nfds = 0;
        fd_set rd;
        FD_ZERO(&rd);
        for (i = 0; i < num_fds; i++) {
            FD_SET(fds[i], &rd);
            nfds = max(nfds, fds[i]);
        }
        err = select(nfds + 1, &rd, NULL, NULL, NULL);
        /* LOG("select %d\n", err); */
        if (err < 0) {
            errprint("select\n");
            continue;
        }
        if (FD_ISSET(fds[SOCK_X], &rd)) {
            /* LOG("X event\n"); */
            /* X event */
            return;
        }
        LOG("Unix domain socket\n");
        for (i = 1; i < num_fds; i++) {
            if (FD_ISSET(fds[i], &rd)) {
                read_unix_socket(i);
                break;
            }
        }
        assert(i != num_fds);
    }
}
