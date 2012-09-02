#ifndef WINS_H_
#define WINS_H_

#include "client.h"

void init_restart_configs();
void mk_backfile_fullpath(char * path);
void apply_window_config(Client* cl);

#endif // ! WINS_H_
