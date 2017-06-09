#ifndef __DROPBOXSERVER__
#define	__DROPBOXSERVER__

#include "../include/dropboxUtil.h"

#define MAXNAME 32
#define MAXFILES 256
#define ERRO -1

typedef struct client {
  int devices[2]; // associado aos dispositivos do usuário
	char userid[MAXNAME]; //  id do usuário no servidor, que deverá ser único. Informado pela linha de comando.
	struct file_info metadata[MAXFILES]; //metadados de cada arquivo que o cliente possui no servidor.
  int logged_in; // cliente está logado ou não.
} client;


void sync_server();
void receive_file(char *file_name, char* file_data);
void sync_dir(char* client_id);
void send_file(char *file_name, int socket);
void auth(int socket);
void receive_command_client(int socket);
void *client_thread(void *new_socket_id);

#endif
