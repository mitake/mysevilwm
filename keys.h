#ifndef __KEYBOARDWM_KEYS_H__
#define __KEYBOARDWM_KEYS_H__

#include <X11/Xlib.h>

#include "events.h"

typedef struct {
    int key;
    void (*ev)(EvArgs);
    EvArgs arg;
} Key;

void keys_init();
int keys_run(XKeyEvent* e);
void keys_run_button(XButtonEvent* e);

#endif // ! __KEYBOARDWM_KEYS_H__
