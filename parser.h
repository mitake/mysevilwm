#ifndef PARSER_H_
#define PARSER_H_

#include "ctrl.h"

/* Parse command and return arg count. Return negative on failure */
int parse_cmd(char* cmd, ctrl_message_callback message_cb, char** args);

#endif /* PARSER_H_ */
