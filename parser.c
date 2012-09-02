#include "parser.h"

#include <stdio.h>
#include <string.h>

static char* scan_str(char* str, const char* stop) {
    for (; *str; str++) {
        if (strchr(stop, *str)) break;
    }
    return str;
}

static void skip_space(char** sp) {
    while (isspace(**sp)) {
        (*sp)++;
    }
}

static int scan_hex(char** sp) {
    int v = 0;
    const char* hex = "0123456789abcdef";
    char c = **sp;
    char* p = strchr(hex, tolower(c));
    if (!p) return -1;
    v = p - hex;
    c = (*sp)[1];
    p = strchr(hex, tolower(c));
    if (!p) return v;
    (*sp)++;
    v = v * 16 + p - hex;
    return v;
}

static int parse_char(char** pp, ctrl_message_callback message_cb) {
    char* p = *pp;
    if (*p == '\\') {
        p++;
        if (*p == 'x') {
            int v;
            p++;
            v = scan_hex(&p);
            if (v < 0) {
                message_cb("Syntax error (invalid \\x)\n");
                return -1;
            }
            *p = v;
        }
        else {
            switch (*p) {
            case 'n':
                *p = '\n';
                break;
            case 'r':
                *p = '\r';
                break;
            case 't':
                *p = '\t';
                break;
            case '0':
                *p = '\0';
                break;
            }
        }
    }
    *pp = p;
    return *p;
}

int parse_cmd(char* cmd, ctrl_message_callback message_cb, char** args) {
    char* p;
    int argc = 1;
    int paren = 0;
    skip_space(&cmd);
    if (!cmd[0]) return -1;

    args[0] = cmd;
    p = scan_str(cmd, " \t(");
    if (!*p) {
        message_cb("Syntax error (no paren)\n");
        return -1;
    }
    paren = *p == '(';
    *p++ = '\0';
    skip_space(&p);
    if (*p == '(') {
        p++;
    }
    else if (!paren) {
        message_cb("Syntax error (no paren)\n");
        return -1;
    }
    for (;;) {
        skip_space(&p);
        if (!*p) {
            message_cb("Syntax error (no close paren)\n");
            return -1;
        }
        if (*p == ')') {
            break;
        }
        if (argc > 10) {
            message_cb("Syntax error (too many arguments)\n");
            return -1;
        }
        if (*p == '"') {
            char* o;
            p++;
            args[argc++] = p;
            o = p;
            while (*p != '"') {
                int c;
                if (!*p) {
                    message_cb("Syntax error (unclosed string literal)\n");
                    return -1;
                }
                c = parse_char(&p, message_cb);
                if (c < 0) {
                    return -1;
                }
                *o = c;
                p++;
                o++;
            }
            *o = '\0';
            p++;
        }
        else if (*p == '\'') {
            p++;
            if (*p == '\'') {
                message_cb("Syntax error (empty char literal)\n");
                return -1;
            }
            if (parse_char(&p, message_cb) < 0) {
                return -1;
            }
            args[argc++] = p;
            p++;
            if (*p != '\'') {
                message_cb("Syntax error (invalid char literal)\n");
                return -1;
            }
            *p++ = '\0';
        }
        else {
            char prev;
            args[argc++] = p;
            p = scan_str(p, " \t,)");
            prev = *p;
            *p = '\0';
            if (prev == ')') break;
            p++;
            if (prev == ',') {
                continue;
            }
        }
        skip_space(&p);
        if (*p == ',') {
            p++;
        }
    }

    return argc;
}

