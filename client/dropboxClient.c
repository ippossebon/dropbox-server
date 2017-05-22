#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include "../include/dropboxUtil.h"

#define PORT 4000


/* Conecta o cliente com o servidor.
host – endereço do servidor
port – porta aguardando conexão */
int connect_server(char *host, int port){
    return 0;
}

/* Sincroniza o diretório “sync_dir_<nomeusuário>” com
o servidor */
void sync_client(){

}

/* Envia um arquivo file para o servidor. Deverá ser
executada quando for realizar upload de um arquivo.
file – path/filename.ext do arquivo a ser enviado */
void send_file(char *file, int socket){

    char buffer[256];
    int aux;

    aux = writeFileToBuffer(file, buffer);
    if (aux != 0){
        printf("Erro ao abrir arquivo.\n");
    }
    printf("Buffer: %s\n", buffer);

    /* Envia conteúdo do buffer pelo socket */
    int r;
	r = write(socket, buffer, strlen(buffer));
    if (r < 0)
		printf("ERROR writing to socket\n");

    bzero(buffer,256);
}


/* Obtém um arquivo file do servidor.
Deverá ser executada quando for realizar download
de um arquivo.
file –filename.ext */
void get_file(char *file){

}

/* Fecha a conexão com o servidor */
void close_connection(){

}

int main(int argc, char *argv[]){

    int socket_id, aux;
    struct sockaddr_in server_address;
    struct hostent *server;
    char buffer[256];

    if (argc < 2) {
		fprintf(stderr,"usage %s hostname\n", argv[0]);
		exit(0);
    }

	server = gethostbyname(argv[1]);
	if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }

    if ((socket_id = socket(AF_INET, SOCK_STREAM, 0)) == -1)
        printf("ERROR opening socket\n");

	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(PORT);
	server_address.sin_addr = *((struct in_addr *)server->h_addr);
	bzero(&(server_address.sin_zero), 8);


	if (connect(socket_id,(struct sockaddr *) &server_address, sizeof(server_address)) < 0)
        printf("ERROR connecting\n");

/*
    printf("Enter the message: ");
    bzero(buffer, 256);
    fgets(buffer, 256, stdin);
*/
    send_file("teste.txt", socket_id);


	/* write in the socket */
/*
    int aux;
	aux = write(socket_id, buffer, strlen(buffer));
    if (aux < 0)
		printf("ERROR writing to socket\n");

    bzero(buffer,256);
*/
	/* read from the socket */

    aux = read(socket_id, buffer, 256);
    if (aux < 0)
		printf("ERROR reading from socket\n");

    printf("%s\n", buffer);

	close(socket_id);



    return 0;
}
