#ifndef EV_HANDLER
#define EV_HANDLER

#include "events.h"

struct ev_handler {
    void * handler;
    const char * name;
};

#endif /* EV_HANDLER */
