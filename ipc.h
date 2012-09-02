#ifndef IPC_H_
#define IPC_H_

void ipc_init_part();
void ipc_init();

/* process IPC events until XEvent comes */
void ipc_process();

#endif /* IPC_H_ */
