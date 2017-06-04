#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <dirent.h>

#include "../include/dropboxServer.h"
#include "../include/dropboxUtil.h"

/* Temos que conferir se não precisamos definir a porta de maneira mais dinâmica */
#define PORT 4000


char username[MAXNAME];

/* Sincroniza o servidor com o diretório “sync_dir_<nomeusuário>”
com o cliente. */
void sync_server(){


}

void sync_dir(char* client_id){

	if(existsClientFolder(client_id)){
		/* Realiza a sincronização*/
		printf("[sync_dir] O cliente possui uma pasta no servidor.\n");
	}
	else{
		printf("[sync_dir] O cliente não possui uma pasta no servidor. Criando...\n");
		char* path = getClientFolderName(client_id);
		mkdir(path, 0700); /* 0700 corresponde ao modo administrador */
	}
}

/* Recebe um arquivo file do cliente.
Deverá ser executada quando for realizar upload de um arquivo.
file – path/filename.ext do arquivo a ser recebido */
void receive_file(char *file_name, char* file_data){
    /* file_data contém o conteúdo do arquivo a ser enviado para o servidor, e
	filename é o nome do arquivo que está sendo enviado. */
	char *full_path = getClientFolderName(username);
	strcat(full_path, file_name);

    //printf("Folder do cliente: %s\n", full_path);
	/* Cria um novo arquivo, na pasta do cliente logado, com o nome do arquivo
	informado, com o conteúdo do arquivo enviado pelo socket. */
	writeBufferToFile(full_path, file_data);
}

/* Envia o arquivo file para o usuário.
Deverá ser executada quando for realizar download de um arquivo.
file – filename.ext */
void send_file(char *file_name, int socket){

	char* full_path = getClientFolderName(username);
	strcat(full_path, file_name);

    char file_data[256];
    bzero(file_data, 256);
	writeFileToBuffer(full_path, file_data);

	int n;
    int size = strlen(file_data);
	n = write(socket, file_data, size);
	if (n < 0){
		printf("Erro ao escrever no socket - Download\n");
	}
}

void list(int socket){

    char* full_path = getClientFolderName(username);
    char buffer[256];
    bzero(buffer, 256);

    DIR *d;
    struct dirent *dir;
    d = opendir(full_path);

    if (d){
        /* Itera em todos os arquivo da pasta do cliente e coloca o nome dos 
            arquivos dentro do buffer separados por '#' ex: file1#file2#file3# */
        while ((dir = readdir(d)) != NULL){
            if (dir->d_type == DT_REG){
                strcat(buffer, dir->d_name);
                strcat(buffer, "#");
            }       
        }
        closedir(d);
    }

	int n;
    int size = strlen(buffer);
    /* Envia para o client os nomes dos arquivos */
	n = write(socket, buffer, size);
	if (n < 0){
		printf("Erro ao escrever no socket - Download\n");
	}

}

void user_verification(int socket){

    char buffer[256];
    bzero(buffer, 256);
    int num_bytes_read, num_bytes_sent;

    /* No buffer vem o userid do cliente que esta tentando conectar */
    num_bytes_read = read(socket, buffer, 256);
    if (num_bytes_read < 0){
        printf("[user_verification] ERROR reading from socket \n");
    }

    strcpy(username, buffer);
	/*
	TODO: verificações de numero de dispositivos, se o cliente já é cadastrado,
	etc

        criar a pasta dele caso n exista?
	*/

    bzero(buffer, 256);
    num_bytes_sent = write(socket, "OK", 3);

	if (num_bytes_sent < 0){
		printf("[user_verification] ERROR writing on socket\n");
	}
}

void receive_command_client(int socket){
    char buffer[256];
    char command[10];
    char file_name[32];
    int num_bytes_read;
    bzero(buffer, 256);
           

		char file_data[256];
		char *p;
		int i = 0;

		/* Recebe comando#filename#arquivo (se existirem) */
		num_bytes_read = read(socket, buffer, 256);

		if (num_bytes_read < 0){
            printf("[receive_command_client] Erro ao ler linha de comando do socket.\n");
        }

		/* Separa o buffer de acordo com as informações necessárias, onde o delimitador é #.*/
		for (p = strtok(buffer,"#"); p != NULL; p = strtok(NULL, "#")){
		  if (i == 0){
				strcpy(command, p);
			}
			else if (i == 1){
				strcpy(file_name, p);
			}
			else{
				strcpy(file_data, p);
			}
			i++;
		}

    /* Realizar a operação de acordo com o comando escolhido. */
    if( strcmp("upload", command) == 0){
        
        receive_file(file_name, file_data);

    }else if( strcmp("download", command) == 0){
        send_file(file_name, socket);

    }else if( strcmp("list", command) == 0){
        list(socket);

    }else if( strcmp("get_sync_dir", command) == 0){
        //sync_client(){
    }
}

int main(int argc, char *argv[]){
	int server_socket_id, new_socket_id;
  struct sockaddr_in server_address, client_address;
	socklen_t client_len;
	char buffer[256];

  /* Cria socket TCP para o servidor. */
	if ((server_socket_id = socket(AF_INET, SOCK_STREAM, 0)) == -1){
		printf("[main] ERROR opening socket\n");
	}

	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(PORT);
	server_address.sin_addr.s_addr = INADDR_ANY;
	bzero(&(server_address.sin_zero), 8);

	if (bind(server_socket_id, (struct sockaddr *) &server_address, sizeof(server_address)) < 0){
		printf("[main] ERROR on binding\n");
	}

  /* Informa que o socket em questões pode receber conexões, 5 indica o
  tamanho da fila de mensagens */
	listen(server_socket_id, 5);

  /* Aceita conexões de clientes */
	client_len = sizeof(struct sockaddr_in);
	if ((new_socket_id = accept(server_socket_id, (struct sockaddr *) &client_address, &client_len)) == -1){
		printf("[main] ERROR on accept\n");
	}

	bzero(buffer, 256);

  /* Faz verificação de usuário. TODO: verificar se existe, se ja está logado, etc */
  user_verification(new_socket_id);

  /* Recebe a linha de comando e redireciona para a função objetivo */
  receive_command_client(new_socket_id);

	close(new_socket_id);
	close(server_socket_id);

	return 0;
}
