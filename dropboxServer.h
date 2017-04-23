#ifndef __DROPBOXSERVER__
#define	__DROPBOXSERVER__

struct client {
    int devices[2]; // associado aos dispositivos do usuário
	char userid[MAXNAME]; //  id do usuário no servidor, que deverá ser único. Informado pela linha de comando.
	struct	file_info[MAXFILES]; //metadados de cada arquivo que o cliente possui no servidor.
    int logged_in; // cliente está logado ou não.
}

struct file_info{
	char name[MAXNAME]; // name[MAXNAME] refere-se ao nome do arquivo
	char extension[MAXNAME]; // extension[MAXNAME] refere-se ao tipo de extensão do arquivo.
	char last_modified[MAXNAME]; // last_modified [MAXNAME] refere-se a data da última mofidicação no arquivo
	int size; // size indica o tamanho do arquivo, em bytes.
}

void sync_server();
void receive_file(char *file);
void send_file(char *file);

#endif
