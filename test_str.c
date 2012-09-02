#include "str.h"

#include <assert.h>
#include <string.h>

int main() {
    Str str = str_new("hoge");
    str_cat(&str, "fuga");
    assert(str.size == 8);
    assert(!strcmp(str.buf, "hogefuga"));
    str_catf(&str, 4, "%d", 20);
    assert(str.size == 10);
    assert(!strcmp(str.buf, "hogefuga20"));
    /* Even if user specifies too few bufsize, we are OK on C99 */
    str_catf(&str, 0, "%d", 20);
    assert(str.size == 12);
    assert(!strcmp(str.buf, "hogefuga2020"));
}
