#include "str.h"

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

static void str_reserve(Str* str, int resv) {
    if (str->resv >= resv) return;
    str->buf = (char*)realloc(str->buf, resv);
    str->resv = resv;
}

Str str_new(const char* s) {
    Str str;
    if (s && *s) {
        str.size = strlen(s);
        str.resv = str.size + 1;
        str.buf = (char*)malloc(str.resv);
        strcpy(str.buf, s);
    }
    else {
        str.size = 0;
        str.resv = 0;
        str.buf = NULL;
    }
    return str;
}

void str_cat_buf(Str* str, const char* buf, int len) {
    int size = str->size + len;
    str_reserve(str, size + 1);
    memcpy(str->buf + str->size, buf, len);
    str->buf[size] = '\0';
    str->size = size;
}

void str_cat(Str* str, const char* cat) {
    str_cat_buf(str, cat, strlen(cat));
}

void str_catf(Str* str, int bufsize, const char* fmt, ...) {
    int written;
    va_list ap;
    str_reserve(str, str->resv + bufsize);
    va_start(ap, fmt);
    written = vsnprintf(str->buf + str->size, bufsize, fmt, ap);
    va_end(ap);
    if (written < 0) {
        /* This should not happen on recent glibc */
        fprintf(stderr, "Too long sprintf\n");
        exit(EXIT_FAILURE);
    }
    if (written >= bufsize) {
        int next_written;
#ifdef STDIO
        fprintf(stderr, "WARNING: bufsize too short\n");
#endif
        str_reserve(str, str->resv + written + 1);
        va_start(ap, fmt);
        next_written = vsnprintf(str->buf + str->size, written + 1, fmt, ap);
        va_end(ap);
        if (next_written < 0 || next_written > written) {
            fprintf(stderr, "Too long sprintf, shouldn't happen\n");
            exit(EXIT_FAILURE);
        }
    }
    str->size += strlen(str->buf + str->size);
}

void str_free(Str* str) {
    free(str->buf);
    str->buf = 0;
    str->size = 0;
    str->resv = 0;
}
