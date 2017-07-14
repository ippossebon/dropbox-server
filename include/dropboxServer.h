#ifndef __DROPBOXSERVER__
#define	__DROPBOXSERVER__

#include "../include/dropboxUtil.h"

#define MAXNAME 32
#define MAXFILES 256
#define ERRO -1
#define MAXCLIENTS 5

void sync_server();
void receive_file(char *file_name, char* file_data, char *userid);
void sync_dir(char* client_id);
void send_file(char *file_name, int socket, char *userid);
void list(int socket, char *userid);
int auth(int socket, char* userid);
void receive_command_client(int socket, char *userid);
void *client_thread(void *new_sockets);
void sync_client(int sync_socket, char* userid);
void sync_server(int sync_socket, char* userid);
void *sync_thread(void *args_sync);
void deleteLocalFile(char* file_name, char* userid);
void close_connection(char* userid);
void send_time(int socket);

#endif
