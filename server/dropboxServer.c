#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "./dropboxServer.h"

/* Temos que conferir se não precisamos definir a porta de maneira mais dinâmica */
#define PORT 4000

/* Sincroniza o servidor com o diretório “sync_dir_<nomeusuário>”
com o cliente. */
void sync_server(){

}

/* Recebe um arquivo file do cliente.
Deverá ser executada quando for realizar upload de um arquivo.
file – path/filename.ext do arquivo a ser recebido */
void receive_file(char *file){

}

/* Envia o arquivo file para o usuário.
Deverá ser executada quando for realizar download de um arquivo.
file – filename.ext */
void send_file(char *file){

}

int main(int argc, char *argv[])
{
	int server_socket_id, new_socket_id;
    struct sockaddr_in server_address, client_address;
	socklen_t client_len;
	char buffer[256];

    /* Cria socket TCP para o servidor. */
	if ((server_socket_id = socket(AF_INET, SOCK_STREAM, 0)) == -1)
        printf("ERROR opening socket");

	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(PORT);
	server_address.sin_addr.s_addr = INADDR_ANY;
	bzero(&(server_address.sin_zero), 8);

	if (bind(server_socket_id, (struct sockaddr *) &server_address, sizeof(server_address)) < 0)
		printf("ERROR on binding");

    /* Informa que o socket em questões pode receber conexões, 5 indica o
    tamanho da fila de mensagens */
	listen(server_socket_id, 5);

    /* Aceita conexões de clientes */
	client_len = sizeof(struct sockaddr_in);
	if ((new_socket_id = accept(server_socket_id, (struct sockaddr *) &client_address, &client_len)) == -1)
		printf("ERROR on accept");

    /* "Limpa" os valores da memória */
	bzero(buffer, 256);

	/* Lê buffer do socket */
    int aux;
	aux = read(new_socket_id, buffer, 256);
	if (aux < 0)
		printf("ERROR reading from socket");
	printf("Here is the message: %s\n", buffer);

	/* write in the socket */
	aux = write(new_socket_id,"I got your message", 18);
	if (aux < 0)
		printf("ERROR writing to socket");

	close(new_socket_id);
	close(server_socket_id);

	return 0;
}
