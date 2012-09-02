#ifndef STR_H_
#define STR_H_

typedef struct {
    char* buf;
    int size;
    int resv;
} Str;

Str str_new(const char* s);

void str_cat_buf(Str* str, const char* buf, int len);

void str_cat(Str* str, const char* cat);

void str_catf(Str* str, int bufsize, const char* fmt, ...);

void str_free(Str* str);

#endif /* STR_H_ */
