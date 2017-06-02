#ifndef __DROPBOXSERVER__
#define	__DROPBOXSERVER__

#define MAXNAME 32
#define MAXFILES 256
#define ERRO -1


struct file_info {
	char name[MAXNAME]; // name[MAXNAME] refere-se ao nome do arquivo
	char extension[MAXNAME]; // extension[MAXNAME] refere-se ao tipo de extensão do arquivo.
	char last_modified[MAXNAME]; // last_modified [MAXNAME] refere-se a data da última mofidicação no arquivo
	int size; // size indica o tamanho do arquivo, em bytes.
}file_info;

struct client {
  int devices[2]; // associado aos dispositivos do usuário
	char userid[MAXNAME]; //  id do usuário no servidor, que deverá ser único. Informado pela linha de comando.
	struct	file_info metadata[MAXFILES]; //metadados de cada arquivo que o cliente possui no servidor.
  int logged_in; // cliente está logado ou não.
}client;


void sync_server();
void sync_dir(char* client_id);
void receive_file(char *file_name, char* file_data);
void send_file(char *file_name, int socket);
void auth(int socket);
void receive_command_client(int socket);
void *client_thread(void *new_socket_id);


#endif
