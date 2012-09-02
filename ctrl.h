#ifndef CTRL_H_
#define CTRL_H_

/* Callback function for ctrl_parse.
 * On success, msg will be NULL.
 */
typedef void (*ctrl_message_callback)(const char* msg);

void ctrl_parse(char* cmds, int* parsed, ctrl_message_callback message_cb);

#endif /* CTRL_H_ */
