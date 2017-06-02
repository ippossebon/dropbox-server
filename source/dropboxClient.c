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


/* Fecha a conexão com o servidor */
void close_connection(){
    /* Avisar o servidor que o cliente está encerrando. */
}


/* Sincroniza o diretório “sync_dir_<nomeusuário>” com o servidor */
void sync_client(){

}


/* Lista os arquivos salvos no servidor */
void list(){

}


/* Envia um arquivo para o servidor. Deverá ser executada quando for realizar upload de um arquivo.
file – path/file_name.ext do arquivo a ser enviado
tag - numero da operação#file#cliente#
UPLOAD */
void send_file(char *file, int socket){

  // int aux;
  //
  // aux = writeFileToBuffer(file, buffer);
  // if (aux != 0){
  //   printf("Erro ao abrir arquivo.\n");
  // }
  //
  // /* Envia conteúdo do buffer pelo socket */
  // int num_bytes_sent;
  // int buffer_size = strlen(buffer);
  // num_bytes_sent = write(socket, buffer, buffer_size);
  //
  // if (num_bytes_sent < 0){
  //   printf("ERROR writing to socket\n");
  // }
  //
  // //printf("[send_file] file: %s - buffer: %s\n", file, buffer );
  // //sendFileThroughSocket(file, buffer, socket);
}


/* Obtém um arquivo file do servidor.
Deverá ser executada quando for realizar download
de um arquivo.
file –file_name.ext
DOWNLOAD */
void get_file(char *file, int socket){
  // int num_bytes_read, num_bytes_sent;
  // char buffer[256];
	// bzero(buffer, 256);
  //
  // num_bytes_sent = write(socket, line, strlen(line));
  // if (num_bytes_sent < 0){
  //   printf("ERROR writing from socket");
  // }
  //
  // num_bytes_read = read(socket, buffer, 256);
  //
  // if (num_bytes_read < 0){
  //   printf("ERROR reading from socket");
  // }
  //
  // writeBufferToFile(file, buffer);
}


int auth(int socket, char* userid){

  int num_bytes_sent, num_bytes_read;
  int buffer_size = strlen(userid);
  char check[8];

  printf("Realizando verificação de usuário... \n\n");

  num_bytes_sent = write(socket, userid, buffer_size);
  if (num_bytes_sent < 0){
    printf("[auth] ERROR writing on socket\n");
    exit(1);
  }

  bzero(check, 8);
  num_bytes_read = read(socket, check, 8);

  if (num_bytes_read < 0){
    printf("[auth] ERROR reading on socket\n");
    exit(1);
  }

  if(strcmp (check, "OK") == 0){
    printf("Usuário autenticado.\n");
    return 0;
  }
  else{
    printf("ERRO: Usuário não autenticado. Excesso de devices conectados.\n");
    return ERRO;
  }
}


int main(int argc, char *argv[]){

  int socket_id;
  char userid[MAXNAME];
  char line[100];
  char file_name[100];
  char command[10];
  char *token;

  /* Teste se todos os argumentos foram informados ao executar o cliente */
  if (argc < 4) {
    printf("Erro. Informe o client_id, servidor e porta.\n");
    exit(0);;
  }

  /* Conecta ao servidor com o endereço e porta informados, retornando o socket_id */
  socket_id = connect_server(argv[2], atoi(argv[3]));
  if(socket_id < 0){
    printf("Erro. Não foi possível conectar ao servidor.\n");
    exit(0);
  }

  /* Userid informado pelo usuário */
  strcpy (userid, argv[1]);

  /* Se o usuário está OK, então pode executar ações */
  if(auth(socket_id, userid) == 0){
    while(1){
      bzero(line, 100);
      bzero(command, 10);
      bzero(file_name,100);

      printf("\n\nDigite seu comando no formato: \nupload <file_name.ext> \ndownload <file_name.ext> \nlist \nget_sync_dir \nexit\n ");
      scanf ("%[^\n]%*c", line);

      /* E obtemos o comando dado, que será utilizado para saber qual função é requerida */
      token = strtok(line, " ");
      strcpy(command, token);
      token = strtok(NULL, " ");
      strcpy(file_name, token);

      /* Realiza a operação solicitada */
      if(strcmp("upload", command) == 0){
        send_file(file_name, socket_id);
      }
      else if(strcmp("download", command) == 0){
        /* TO DO: fazer funcionar*/
        get_file(file_name, socket_id);
      }
      else if(strcmp("list", command) == 0){
        //falta uma função para a list
        //enviar comando
      }
      else if(strcmp("get_sync_dir", command) == 0){
        //sync_client(){
      }
      else if(strcmp("exit", command) == 0){
        /* Informa pro servidor que quer desconectar */
        printf("Adeus!\n");
        break;
      }
      else{
        printf("Comando inválido.\n");
      }
    }
  }

  /* Encerra a conexão com o servidor */
  close_connection();
	close(socket_id);
  return 0;
}
