#include "parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

char* cmdbuf;
const char* msg;

void cb(const char* p) {
    msg = p;
}

int run_cmd(const char* s, char** args) {
    int argc, i;
    msg = NULL;
    strcpy(cmdbuf, s);
    argc = parse_cmd(cmdbuf, cb, args);
    if (msg) {
        printf("err=%s", msg);
        assert(argc == -1);
    }
    else {
        printf("argc=%d\n", argc);
    }
    for (i = 0; i < argc; i++) {
        printf("args[%d]=\"%s\"\n", i, args[i]);
    }
    return argc;
}

int main() {
    char* args[11];

    cmdbuf = (char*)malloc(4096);

    assert(1 == run_cmd("SHUTDOWN()", args));
    assert(!strcmp(args[0], "SHUTDOWN"));
    assert(msg == NULL);

    assert(1 == run_cmd("SHUTDOWN( )", args));
    assert(!strcmp(args[0], "SHUTDOWN"));
    assert(msg == NULL);

    assert(1 == run_cmd("SHUTDOWN ( )", args));
    assert(!strcmp(args[0], "SHUTDOWN"));
    assert(msg == NULL);

    assert(1 == run_cmd("SHUTDOWN ();", args));
    assert(!strcmp(args[0], "SHUTDOWN"));
    assert(msg == NULL);

    assert(2 == run_cmd("CMD(hoge);", args));
    assert(!strcmp(args[0], "CMD"));
    assert(!strcmp(args[1], "hoge"));
    assert(msg == NULL);

    assert(2 == run_cmd("CMD (hoge);", args));
    assert(!strcmp(args[0], "CMD"));
    assert(!strcmp(args[1], "hoge"));
    assert(msg == NULL);

    assert(2 == run_cmd("CMD (hoge );", args));
    assert(!strcmp(args[0], "CMD"));
    assert(!strcmp(args[1], "hoge"));
    assert(msg == NULL);

    assert(2 == run_cmd("CMD ( hoge );", args));
    assert(!strcmp(args[0], "CMD"));
    assert(!strcmp(args[1], "hoge"));
    assert(msg == NULL);

    assert(2 == run_cmd("CMD ( hoge);", args));
    assert(!strcmp(args[0], "CMD"));
    assert(!strcmp(args[1], "hoge"));
    assert(msg == NULL);

    assert(2 == run_cmd("CMD(\"hoge\");", args));
    assert(!strcmp(args[0], "CMD"));
    assert(!strcmp(args[1], "hoge"));
    assert(msg == NULL);

    assert(2 == run_cmd("CMD(\"hoge\" );", args));
    assert(!strcmp(args[0], "CMD"));
    assert(!strcmp(args[1], "hoge"));
    assert(msg == NULL);

    assert(2 == run_cmd("CMD( \"hoge\");", args));
    assert(!strcmp(args[0], "CMD"));
    assert(!strcmp(args[1], "hoge"));
    assert(msg == NULL);

    assert(2 == run_cmd("CMD   ( \"hoge\"  );", args));
    assert(!strcmp(args[0], "CMD"));
    assert(!strcmp(args[1], "hoge"));
    assert(msg == NULL);

    assert(2 == run_cmd("CMD   ( \"\"  );", args));
    assert(!strcmp(args[0], "CMD"));
    assert(!strcmp(args[1], ""));
    assert(msg == NULL);

    assert(-1 == run_cmd("CMD(\"", args));

    assert(2 == run_cmd("CMD(\"ho\\n\\r\\tge\\x30\");", args));
    assert(!strcmp(args[0], "CMD"));
    assert(!strcmp(args[1], "ho\n\r\tge0"));
    assert(msg == NULL);

    assert(2 == run_cmd("CMD('h');", args));
    assert(!strcmp(args[0], "CMD"));
    assert(!strcmp(args[1], "h"));
    assert(msg == NULL);

    assert(-1 == run_cmd("CMD('');", args));

    assert(2 == run_cmd("CMD('\\n');", args));
    assert(!strcmp(args[0], "CMD"));
    assert(!strcmp(args[1], "\n"));
    assert(msg == NULL);

    assert(-1 == run_cmd("SHUTDOWN", args));
    assert(-1 == run_cmd("SHUTDOWN(", args));
    assert(-1 == run_cmd("SHUTDOWN)", args));
    assert(-1 == run_cmd("SHUTDOWN('aa')", args));

    assert(3 == run_cmd("CMD(hoge,fuga);", args));
    assert(!strcmp(args[0], "CMD"));
    assert(!strcmp(args[1], "hoge"));
    assert(!strcmp(args[2], "fuga"));
    assert(msg == NULL);

    assert(3 == run_cmd("CMD(hoge, fuga);", args));
    assert(!strcmp(args[0], "CMD"));
    assert(!strcmp(args[1], "hoge"));
    assert(!strcmp(args[2], "fuga"));
    assert(msg == NULL);

    assert(3 == run_cmd("CMD ( hoge , fuga );", args));
    assert(!strcmp(args[0], "CMD"));
    assert(!strcmp(args[1], "hoge"));
    assert(!strcmp(args[2], "fuga"));
    assert(msg == NULL);

    assert(5 == run_cmd("CMD(hoge ,\"fuga\",'a',32);", args));
    assert(!strcmp(args[0], "CMD"));
    assert(!strcmp(args[1], "hoge"));
    assert(!strcmp(args[2], "fuga"));
    assert(!strcmp(args[3], "a"));
    assert(!strcmp(args[4], "32"));
    assert(msg == NULL);

    assert(5 == run_cmd("CMD ( hoge , \"fuga\", 'a', 32  );", args));
    assert(!strcmp(args[0], "CMD"));
    assert(!strcmp(args[1], "hoge"));
    assert(!strcmp(args[2], "fuga"));
    assert(!strcmp(args[3], "a"));
    assert(!strcmp(args[4], "32"));
    assert(msg == NULL);

    assert(10 == run_cmd("CMD(0,1,2,3,4,5,6,7,8);", args));
    assert(11 == run_cmd("CMD(0,1,2,3,4,5,6,7,8,9);", args));
    assert(-1 == run_cmd("CMD(0,1,2,3,4,5,6,7,8,9,10);", args));

    assert(10 == run_cmd("WIN(\"XTerm\", 1, 2,30,484, 316, 1, 1, '\\0');",
                         args));

    free(cmdbuf);
}
