#ifndef __DROPBOXSERVER__
#define	__DROPBOXSERVER__

#include "../include/dropboxUtil.h"

#define MAXNAME 32
#define MAXFILES 256
#define ERRO -1

void sync_server();
void receive_file(char *file_name, char* file_data);
void sync_dir(char* client_id);
void send_file(char *file_name, int socket);
void auth(int socket);
void receive_command_client(int socket);
void *client_thread(void *new_socket_id);
void get_sync_dir(char* userid);
int createSocket();

#endif
