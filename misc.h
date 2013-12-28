#ifndef MISC_H_
#define MISC_H_

#include "ctrl.h"

void backup_argv(int argc, char ** argv);
void add_argv(int argc, char ** args, ctrl_message_callback message_cb);
void get_argv(int argc, char ** args, ctrl_message_callback message_cb);
void clear_argv(int argc, char ** args, ctrl_message_callback message_cb);

void throwAllUnmapEvent();
void restart();
#ifdef SANITY
void sanity_check();
#endif

#endif // ! MISC_H_
