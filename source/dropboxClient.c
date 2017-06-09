#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include "../include/dropboxUtil.h"
#include <errno.h>
#include <libgen.h>

/*Globais*/

file_node* current_files; //Lista de arquivos no diretório do compartilhado do usuário
char sync_dir[255]; //Variável com o nome da pasta sync do usuário


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


/* Sincroniza o diretório “sync_dir_<nomeusuário>” com
o servidor */
void sync_client(){
}


/* Envia um arquivo file para o servidor. Deverá ser
executada quando for realizar upload de um arquivo.
file – path/filename.ext do arquivo a ser enviado
UPLOAD */
void send_file(char *file, char* buffer, int socket){

  int aux;

  aux = writeFileToBuffer(file, buffer);
  if (aux != 0){
    printf("Erro ao abrir arquivo.\n");
  }

  /* Envia conteúdo do buffer pelo socket */
  int num_bytes_sent;
  int buffer_size = strlen(buffer);
	num_bytes_sent = write(socket, buffer, buffer_size);

  if (num_bytes_sent < 0){
    printf("ERROR writing to socket\n");
  }

  //printf("[send_file] file: %s - buffer: %s\n", file, buffer );
  //sendFileThroughSocket(file, buffer, socket);
}


/* Obtém um arquivo file do servidor.
Deverá ser executada quando for realizar download
de um arquivo.
file –filename.ext
DOWNLOAD */
void get_file(char *file, char* line, int socket){
  int num_bytes_read, num_bytes_sent;
  char buffer[256];
	bzero(buffer, 256);

  num_bytes_sent = write(socket, line, strlen(line));
  if (num_bytes_sent < 0){
    printf("ERROR writing from socket");
  }

  num_bytes_read = read(socket, buffer, 256);

  if (num_bytes_read < 0){
    printf("ERROR reading from socket");
  }

  writeBufferToFile(file, buffer);
}

void list(char* line, int socket){
  int num_bytes_read, num_bytes_sent;
  char buffer[256];
	bzero(buffer, 256);

  num_bytes_sent = write(socket, line, strlen(line)); //enviar o #list#
  if (num_bytes_sent < 0){
    printf("[list] ERROR writing from socket");
  }

  /* Lê o nome dos arquivos que vem no buffer no formato: file1#file2#file# */
  num_bytes_read = read(socket, buffer, 256);

  if (num_bytes_read < 0){
    printf("[list] ERROR reading from socket");
  }

  printf("*** Arquivo(s): ***\n");
  char *p;
  for (p = strtok(buffer,"#"); p != NULL; p = strtok(NULL, "#")){
    printf(">> %s\n", p);
  }
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
  char buffer[256];

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

  /* Se o usuário está OK, então pode executar ações */
  if(auth(socket_id, userid) == 0){
    char command[10];
    char fileName[100];
    char line[110];

    //Atualiza pasta do usuário
    strcpy(sync_dir, "sync_dir_");
    strcat(sync_dir, userid);
    printf("Seu diretório sincronizado é [%s]\n",sync_dir);
    //Monta a lista inicial de arquivos do diretório
    current_files = fn_create_from_path(sync_dir);
    fn_print(current_files);

    while(1){

      bzero(line, 110);
      bzero(buffer, 256);
      bzero(command, 10);
      bzero(fileName,100);

      printf("\n\nDigite seu comando no formato: \nupload <filename.ext> \ndownload <filename.ext> \nlist \nget_sync_dir \nexit\n ");
      scanf ("%[^\n]%*c", line);

      int i=0;
      char *p;
      for (p = strtok(line," "); p != NULL; p = strtok(NULL, " ")){
        if (i == 0){
          strcpy(command, p);
        }
        else{
          strcpy(fileName, p);
        }
        i++;
      }

      /* Monta a linha de comando no formato: comando#nome_arquivo#conteudo_arquivo */
      char* name = basename(fileName);
      strcat(buffer, command);
      strcat(buffer, "#");
      strcat(buffer, name);
      strcat(buffer, "#");

      /* Realiza a operação solicitada */
      if( strcmp("upload", command) == 0){
        send_file(fileName, buffer, socket_id);
      }
      else if( strcmp("download", command) == 0){
        /*TODO: fazer funcionar*/
        get_file(fileName, buffer, socket_id);
      }
      else if( strcmp("list", command) == 0){
        list(buffer, socket_id);
      }
      else if( strcmp("sync", command) == 0){
        sync_client();
      }
      else if( strcmp("exit", command) == 0){
        printf("Adeus! \n");
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
