#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

/* Conecta o cliente com o servidor.
host – endereço do servidor
port – porta aguardando conexão */
int connect_server(char *host, int port){
    int socket_id;
    struct sockaddr_in server_address;
    struct hostent *server;

    /* Verifica se o servidor informado é válido */
    server = gethostbyname(host);
    if (server == NULL) {
        printf("Erro. Endereço informado inválido.\n");
        return ERRO;
    }

    if ((socket_id = socket(AF_INET, SOCK_STREAM, 0)) == -1){
        printf("Erro ao iniciar o socket.\n");
        return ERRO;
    }

	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(port);
	server_address.sin_addr = *((struct in_addr *)server->h_addr);
	bzero(&(server_address.sin_zero), 8);

	if (connect(socket_id,(struct sockaddr *) &server_address, sizeof(server_address)) < 0){
        printf("Erro ao conectar.\n");
        return ERRO;
    }

    return socket_id;
}

/* Sincroniza o diretório “sync_dir_<nomeusuário>” com
o servidor */
void sync_client(){

}

/* Envia um arquivo file para o servidor. Deverá ser
executada quando for realizar upload de um arquivo.
file – path/filename.ext do arquivo a ser enviado */
void send_file(char *file){

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
    int socket_id, n;
    struct sockaddr_in server_address;
    struct hostent *server;
    char buffer[256];

    char userid[MAXNAME];

    /* Teste se todos os argumentos foram informados ao executar o cliente */
    if (argc < 4) {
        printf("Erro. Informe o id do cliente e endereço e porta do servidor.\n");
		exit(0);
    }

    /* Conecta ao servidor com o endereço e porta informados, retornando o socket_id */
    socket_id = connect_server(argv[2], atoi(argv[3]));
    if(socket_id < 0){
        printf("Erro. Não foi possível conectar ao servidor.\n");
		exit(0);
    }

    /* Userid informado pelo usuário */
    strcpy (userid, argv[1]);
    printf("Userid informado: %s\n", userid);

    printf("Enter the message: ");
    bzero(buffer, 256);
    fgets(buffer, 256, stdin);

	/* write in the socket */
    int aux;
	aux = write(socket_id, buffer, strlen(buffer));
    if (aux < 0)
		printf("ERROR writing to socket\n");

    bzero(buffer,256);

	/* read from the socket */
    aux = read(socket_id, buffer, 256);
    if (n < 0)
		printf("ERROR reading from socket\n");

    printf("%s\n", buffer);

	close(socket_id);
    return 0;
}
