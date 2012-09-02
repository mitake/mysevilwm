#ifndef KEYBOARDWM_IGNORE_H__
#define KEYBOARDWM_IGNORE_H__

#include "ctrl.h"

/* extern char* ignore[]; */
int is_ignore(Client* c);
void ignore_init(void);
void add_ignore(int argc, char ** args, ctrl_message_callback message_cb);
void del_ignore(int argc, char ** args, ctrl_message_callback message_cb);


#endif // ! KEYBOARDWM_IGNORE_H__
