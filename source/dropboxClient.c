#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include "../include/dropboxUtil.h"

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
file – path/filename.ext do arquivo a ser enviado
tag - numero da operação#file#cliente#
UPLOAD */
void send_file(char *file, char *tag, int socket){
    sendFileThroughSocket(file, tag, socket);
}


/* Obtém um arquivo file do servidor.
Deverá ser executada quando for realizar download
de um arquivo.
file –filename.ext
DOWNLOAD */
void get_file(char *file, int socket){
    receiveFileThroughSocket(file, socket);
}

/* Fecha a conexão com o servidor */
void close_connection(){

}

int main(int argc, char *argv[]){

    int socket_id, aux;
    char buffer[256];
    char userid[MAXNAME];

    /* Teste se todos os argumentos foram informados ao executar o cliente */
    if (argc < 4) {
        printf("Usage: client_id host port.\n");
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

    char line[110];
    char command[10];
    char fileName[100];
    char tag[111];
    while(1){
        bzero(line, 110);
        bzero(command, 10);
        bzero(fileName,100);
        bzero(tag, 111);
        printf("\nDigite seu comando no formato: \nupload <filename.ext> \ndownload <filename.ext> \nlist \nget_sync_dir \nexit\n ");
        //fgets(line, 110, stdin);
        scanf ("%[^\n]%*c", line);

        int i, flag = 0, count=0;
        for(i=0; i<strlen(line); i++){            
            if(line[i] == ' '){ //Se for igual, começa o nome do arquivo.
                flag = 1;
            }

            if(flag == 0){ //Lendo nome do comando 
                command[i] = line[i]; 
            }else{ //Lendo nome do arquivo
                if(line[i] != ' '){
                    fileName[count] = line[i];
                    count++;
                }
            }
        }

        //printf("Comando: %s  filename: %s\n\n", fileName, fileName);

        //Enviar comunicação p servidor no formato: operação#file#cliente#
        // upload = 1, download = 2, list = 3, get_sync_dir = 4, exit = 5 (nem precisa)
        if( strcmp("upload", command) == 0){
            strcat(tag, "1#"); strcat(tag, fileName); strcat(tag, "#"), strcat(tag, userid), strcat(tag, "#");
           //printf("tag: %s\n", tag);
            send_file(fileName, tag, socket_id);

        }else if( strcmp("download", command) == 0){
            strcat(tag, "2#"); strcat(tag, fileName); strcat(tag, "#"), strcat(tag, userid), strcat(tag, "#");
            //get_file(char *file, int socket){

        }else if( strcmp("list", command) == 0){
            strcat(tag, "3#"); strcat(tag, fileName); strcat(tag, "#"), strcat(tag, userid), strcat(tag, "#");
            //falta uma função para a list

        }else if( strcmp("get_sync_dir", command) == 0){
            strcat(tag, "4#"); strcat(tag, fileName); strcat(tag, "#"), strcat(tag, userid), strcat(tag, "#");
            //sync_client(){

        }else if( strcmp("exit", command) == 0){
            printf("Adeus! \n");
            break;

        }else{
            printf("Comando inválido.\n");

        }


    }



    aux = read(socket_id, buffer, 256);
    if (aux < 0)
		printf("ERROR reading from socket\n");

    printf("%s\n", buffer);

	close(socket_id);


    return 0;
}
