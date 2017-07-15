#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include "../include/dropboxUtil.h"
#include "../include/dropboxClient.h"
#include <errno.h>
#include <libgen.h>
#include <sys/stat.h>
#include <time.h>
#include <dirent.h>


/* Globais */
char host[128];
int port;
char userid[MAXNAME];
SSL_METHOD *method_sync; //inicializa um ponteiro para armazenar a estrutura do SSL que descreve as funções internas, necessário para criar o contexto
SSL_METHOD *method_cmd;
SSL_CTX *ctx_sync; //ponteiro para a estrutura do contexto
SSL_CTX *ctx_cmd; //ponteiro para a estrutura do contexto
SSL *ssl_sync; //usado para as funções de descrição e anexação do SSL ao socket
SSL *ssl_cmd; //usado para as funções de descrição e anexação do SSL ao socket

/* Thread para a sincronização do cliente */
pthread_t s_thread;

file_node* current_files; //Lista de arquivos no diretório do compartilhado do usuário
file_node* real_current_files;
char sync_dir[255]; //Variável com o nome da pasta sync do usuário
int sync_socket;

/*
  Conecta o cliente com o servidor.
  host – endereço do servidor
  port – porta aguardando conexão
*/
int connect_server(char *host, int port){
  int socket_id;
  struct sockaddr_in server_address;
  struct hostent *server;

  /* Verifica se o servidor informado é válido */
  server = gethostbyname(host);
  if (server == NULL) {
    printf("[connect_server] Erro. Endereço informado inválido.\n");
    return ERRO;
  }

  if ((socket_id = socket(AF_INET, SOCK_STREAM, 0)) == -1){
    printf("[connect_server] Erro ao iniciar o socket.\n");
    return ERRO;
  }

  server_address.sin_family = AF_INET;
  server_address.sin_port = htons(port);
  server_address.sin_addr = *((struct in_addr *)server->h_addr);
  bzero(&(server_address.sin_zero), 8);

  if (connect(socket_id,(struct sockaddr *) &server_address, sizeof(server_address)) < 0){
    printf("[connect_server] Erro ao conectar.\n");
    return ERRO;
  }

  return socket_id;
}


/* Avisar o servidor que o cliente está encerrando. */
void close_connection(char* buffer, SSL *ssl){
  /* Mata a thread de sincronização */
  pthread_cancel(s_thread);

  /* Envia conteúdo do buffer pelo socket */
  int num_bytes_sent;
  int buffer_size = strlen(buffer);
  num_bytes_sent = SSL_write(ssl, buffer, buffer_size);

  if (num_bytes_sent < 0){
    printf("ERROR writing to socket\n");
  }
}

/* Envia um arquivo file para o servidor. Deverá ser
executada quando for realizar upload de um arquivo.
file – path/filename.ext do arquivo a ser enviado
UPLOAD */
void send_file(char *file, char* buffer, SSL *ssl){

  int aux;

  aux = writeFileToBuffer(file, buffer);
  if (aux != 0){
    printf("Erro ao abrir arquivo.\n");
  }

  /* Envia conteúdo do buffer pelo socket */
  int num_bytes_sent;
  num_bytes_sent = SSL_write(ssl, buffer, BUF_SIZE);

  if (num_bytes_sent < 0){
    printf("ERROR writing to socket\n");
  }
}


/* Obtém um arquivo file do servidor.
Deverá ser executada quando for realizar download de um arquivo.
file –filename.ext
DOWNLOAD */
void get_file(char *file, SSL *ssl){
    int num_bytes_read;
    char buffer[BUF_SIZE];
    bzero(buffer, BUF_SIZE);

    num_bytes_read = SSL_read(ssl, buffer, BUF_SIZE);

    if (num_bytes_read < 0){
        printf("ERROR reading from socket");
    }

    writeBufferToFile(file, buffer);
}

/* Sincroniza o diretório “sync_dir_<nomeusuário>” com
o servidor */
void sync_client(int commands_socket){

  /* Informa ao server que vai sincronizar (avisa quais foram as modificações
  realizadas localmente)*/
  char buffer[BUF_SIZE];
  bzero(buffer, BUF_SIZE);
  strcpy(buffer, "start client sync");

  SSL_write(ssl_sync, buffer, BUF_SIZE);

  /* Aqui estará a nova lista com os arquivos atuais/reais */
  real_current_files = (file_node*) fn_create_from_path_server_time(sync_dir, commands_socket);
  file_node* node;

  /* Para cada arquivo "real/nova" bucamos na lista "antiga" se ele existe.
    Se encontrar: verifica se foi modificado. Se tiver sido modificado então = UPLOAD do arquivo.
    Se não encontrar: significa que esse arquivo foi adicionado = UPLOAD arquivo */
  for (node = real_current_files; node !=NULL; node = node->next) {
    /* Retorna o file_info do nodo se encontrado, se não retorna NULL */
    file_info* old_file = fn_find(current_files, node->data->name);
    int send = 1;

    if(old_file != NULL){ //encontrou o arquivo, verifica se foi modificado
       if(strcmp(old_file->last_modified, node->data->last_modified) == 0){
           send =0;
       }
    }

    if (send == 1){ //Não encontrou o arquivo, então ele foi adicionado
      /* Deve enviar o arquivo para o servidor. Análogo a uma operação UPLOAD.*/
      bzero(buffer, BUF_SIZE);
      sprintf(buffer, "upload#%s", node->data->name);
      SSL_write(ssl_sync, buffer, BUF_SIZE);

      char send_file_buffer[BUF_SIZE];
      bzero(send_file_buffer, BUF_SIZE);

      char full_path[500];
      sprintf(full_path, "%s/%s", sync_dir, node->data->name);

      send_file(full_path, send_file_buffer, ssl_sync);
    }

  }

  /* Para cada arquivo "antigo" bucamos na lista "real/nova" se ele existe.
    Se encontrar: a verificação de cima já está fazendo o que precisa ser feito, então não faria nada nesse caso.
    Se não encontrar: significa que esse arquivo foi deletado = DELETE arquivo */
  for (node = current_files; node !=NULL; node = node->next) {
    /* Retorna o file_info do nodo se encontrado, se não retorna NULL */
    file_info* old_file = fn_find(real_current_files, node->data->name);

    if(old_file == NULL){ //nao encontrou o arquivo = deletado
        bzero(buffer, BUF_SIZE);
        sprintf(buffer, "delete#%s", node->data->name);
        SSL_write(ssl_sync, buffer, BUF_SIZE);
    }
  }

  fn_clear(current_files);
  /* current_file agora aponta para a real_current_files */
  current_files = real_current_files;

  /* Avisa o servidor que terminou de fazer o seu sync. */
  bzero(buffer, BUF_SIZE);
  strcat(buffer, "end client sync");
  SSL_write(ssl_sync, buffer, BUF_SIZE);
}

/* Recebe as modificações que foram feitas localmente pelo server */
void sync_server(int commands_socket){

    char buffer[BUF_SIZE];
    bzero(buffer, BUF_SIZE);
    file_node *node;
    // Enviando uma lista de nomes para o servidor no formato: nome_arquivo1#dataDeModificação#nome_arquivo2#data...#
    for (node = current_files; node !=NULL; node = node->next) {
        strcat(buffer, node->data->name);
        strcat(buffer, "#");
        strcat(buffer, node->data->last_modified);
        strcat(buffer, "#");
    }
    SSL_write(ssl_sync, buffer, BUF_SIZE);

    while(1){
        bzero(buffer, BUF_SIZE);
        SSL_read(ssl_sync, buffer, BUF_SIZE);
        if(strcmp(buffer, "end server sync")== 0){
            break;
        }
        char *save;
        char *command = strtok_r(buffer,"#", &save); //upload
        char* file_name = strtok_r(NULL, "#", &save);

        if(strcmp(command, "delete")==0){
            char rm[500];
            sprintf(rm, "rm \"%s/%s\"", sync_dir, file_name);
            system(rm);
        }else if(strcmp(command, "upload")==0){
            char full_path[500];
            sprintf(full_path, "%s/%s", sync_dir, file_name);
            get_file(full_path, ssl_sync);
        }
    }

    // A current está atualizando para que não dar erro.
    fn_clear(current_files);
    current_files = fn_create_from_path_server_time(sync_dir, commands_socket);
}

void insertSSLIntoSocketSync(int socket) {
  ssl_sync = SSL_new(ctx_sync);
  SSL_set_fd(ssl_sync, socket);

  if (SSL_connect(ssl_sync) == -1)
      ERR_print_errors_fp(stderr);
}

void insertSSLIntoSocketCmd(int socket) {
  ssl_cmd = SSL_new(ctx_cmd);
  SSL_set_fd(ssl_cmd, socket);
  if (SSL_connect(ssl_cmd) == -1)
      ERR_print_errors_fp(stderr);
  else {
      //GG
      X509 *cert;
      char *line;
      cert = SSL_get_peer_certificate(ssl_cmd);
      if (cert != NULL) {
          line = X509_NAME_oneline(X509_get_subject_name(cert), 0, 0);
          printf("Subject: %s\n", line);
          free(line);
          line = X509_NAME_oneline(X509_get_issuer_name(cert), 0, 0);
          printf("Issuer: %s\n", line);
      }
  }
}

void list(char* line, SSL *ssl){
  int num_bytes_read, num_bytes_sent;
  char buffer[256];
  bzero(buffer, 256);

  num_bytes_sent = SSL_write(ssl, line, strlen(line)); //enviar o #list#
  if (num_bytes_sent < 0){
    printf("[list] ERROR writing from socket");
  }

  /* Lê o nome dos arquivos que vem no buffer no formato: file1#file2#file# */
  num_bytes_read = SSL_read(ssl, buffer, 256);

  if (num_bytes_read < 0){
    printf("[list] ERROR reading from socket");
  }


  if(strcmp(buffer, "empty") == 0){
      printf("Este diretório está vazio.\n");
  }
  else{
      printf("*** Arquivo(s): ***\n");
      char *p, *save;
      for (p = strtok_r(buffer,"#", &save); p != NULL; p = strtok_r(NULL, "#", &save)){
        printf(">> %s\n", p);
      }
  }
}


int auth(SSL *ssl, char* userid){
  printf("Realizando verificação de usuário... \n\n");

  int num_bytes_sent, num_bytes_read;
  int buffer_size = strlen(userid);
  char buffer[256];

  num_bytes_sent = SSL_write(ssl, userid, buffer_size);

  if (num_bytes_sent < 0){
    printf("[auth] ERROR writing on socket\n");
    exit(1);
  }

  bzero(buffer, 256);
  num_bytes_read = SSL_read(ssl, buffer, 256);

  if (num_bytes_read < 0){
    printf("[auth] ERROR reading on socket\n");
    exit(1);
  }

  if(strcmp (buffer, "OK") == 0){
    printf("Usuário autenticado.\n");
    return 1;
  }
  else if (strcmp(buffer, "NOT OK") == 0){
    printf("ERRO: Usuário não autenticado. Excesso de devices conectados.\n");
    return ERRO;
  }
  else{
      printf("ERRO: Mensagem não reconhecida\n");
      return ERRO;
  }
}

int check_sync_dir(){
    /* Verifica se existe um diretório chamado sync_dir_userid, se não existir,
    cria. */
    char folder_name[128];
    bzero(folder_name, 128);
    strcat(folder_name, "./sync_dir_");
    strcat(folder_name, userid);

    if(existsFolder(folder_name) != 1){
        mkdir(folder_name, 0700);
    }
    return SUCESSO;
}


void *sync_thread(void *sockets){
    /* Uns casts muito loucos */
    arg_struct *args = sockets;
	int socket_id = args->socket_id;
    sync_socket = args->sync_socket;

    while(1){
        sync_client(socket_id);
        sync_server(socket_id);
        sleep(10);
    }

    return 0;
}

/*
Requisita o horário do servidor na forma de um timestamp
Unix.
O resultado será usado pela aplicação cliente para calcular
sua nova referência temporal (a ser utilizada nos registros
de arquivos)
*/
char* get_timestamp_server(int socket){

    int num_bytes_read, num_bytes_sent;
    char buffer[256];
    char* command = "time#";
    bzero(buffer, 256);

    /* Pega a hora atual para determinar quanto tempo vai demorar a requisição.*/
    time_t now;
    struct tm *before_request_time, *after_request_time;

    now = time (NULL);
    before_request_time = localtime (&now);

    /* Envia o comando time# */
    num_bytes_sent = write(socket, command, strlen(command));
    if (num_bytes_sent < 0){
      printf("[get_timestamp_server] ERROR writing on socket");
    }

    /* Lê o timestamp do servidor que vem no formato aaaa.mm.dd hh:mm:ss */
    num_bytes_read = read(socket, buffer, 256);

    if (num_bytes_read < 0){
      printf("[get_timestamp_server] ERROR reading from socket");
    }

    now = time (NULL);
    after_request_time = localtime (&now);

    //printf("Timestamp que veio do server: %s\n", buffer);

    struct tm *time_server = malloc(sizeof(struct tm));
    strptime(buffer, "%Y.%m.%d %H:%M:%S", time_server);
    time_server->tm_mon += 1; // não sei por que

    /* Aplica a fórmula T_cliente = T_servidor + (T1 - t0)/2*/
    double delta_time = difftime(mktime(after_request_time), mktime(before_request_time));
    delta_time = delta_time / 2;

    time_t new_time = delta_time + mktime(time_server);
    struct tm *time_client = malloc(sizeof(struct tm));
    time_client = localtime(&new_time);

    /* Coloca a data no formato: aaaa.mm.dd hh:mm:ss */
    char *timestamp = malloc(sizeof(char) * 36);
    bzero(timestamp, 36);

    time_client->tm_hour += 1; // não sei por que, tá louco

    char seconds[2];
    sprintf(seconds, "%d", time_client->tm_sec);
    char minutes[2];
    sprintf(minutes, "%d", time_client->tm_min);
    char hour[2];
    sprintf(hour, "%d", time_client->tm_hour);
    char day[2];
    sprintf(day, "%d", time_client->tm_mday);
    char month[2];
    sprintf(month, "%d", time_client->tm_mon + 1);
    char year[2];
    sprintf(year, "%d", time_client->tm_year + 1900);

    strcat(timestamp, year);
    strcat(timestamp, ".");
    strcat(timestamp, month);
    strcat(timestamp, ".");
    strcat(timestamp, day);
    strcat(timestamp, " ");
    strcat(timestamp, hour);
    strcat(timestamp, ":");
    strcat(timestamp, minutes);
    strcat(timestamp, ":");
    strcat(timestamp, seconds);

    //printf("Timestamp final no cliente: %s\n", timestamp);

    return timestamp;
}

/* Cria um file_set a partir dos arquivos de um caminho indicado por path,
utilizando o timestamp recebido do servidor, sobre o algoritmo de Cristian.
*/
file_node* fn_create_from_path_server_time(char* path, int socket) {

   file_node* list = fn_create();

   DIR* d = opendir(path);
   if (d) {
      struct dirent *dir;
      while ((dir = readdir(d)) != NULL) {
         if (dir->d_type == DT_REG) { //verifica se é um arquivo
            char* filename = dir->d_name;
            struct stat attr; //Essa estrutura armazena os atributos do arquivo
            char fullpath[256];
            sprintf(fullpath, "%s/%s",path,filename);
            if (stat(fullpath,&attr)) {
               perror(fullpath);
               exit(-1);
            } else {
               //Adiciona um file_info a lista
               file_info* file = malloc(sizeof(file_info));
               strcpy(file->name, filename);

               /* Pega o horário atualizado de acordo com o horário do server. */
               char *timestamp = get_timestamp_server(socket);
               strcpy(file->last_modified, timestamp);
               file->size = (int)attr.st_size;
               strcpy(file->extension, "unknown"); //TODO arrumer isso para pegar a extensão do arquivo se houver
               list = fn_add(list,file);
            }
         }
      }
      closedir(d);
   }
   return list;
}

void createMethodCTXSync() {
  method_sync = SSLv23_client_method();
  ctx_sync = SSL_CTX_new(method_sync);
  if (ctx_sync == NULL) {
      ERR_print_errors_fp(stderr);
      abort();
  }
}

void createMethodCTXCmd() {
  method_cmd = SSLv23_client_method();
  ctx_cmd = SSL_CTX_new(method_cmd);
  if (ctx_cmd == NULL) {
      ERR_print_errors_fp(stderr);
      abort();
  }
}

int main(int argc, char *argv[]){
  int socket_id;

  /* Configurando SSL */
  initializeSSL();
  createMethodCTXCmd();
  createMethodCTXSync();

  /* Testa se todos os argumentos foram informados ao executar o cliente */
  if (argc < 4) {
    printf("Usage: <client_id> <host> <port>.\n");
	exit(0);
  }

  strcpy (userid, argv[1]);
  strcpy(host, argv[2]);
  port = atoi(argv[3]);
  printf("Tentando conectar: %s %s %d\n", userid, host, port);
  /* Conecta ao servidor com o endereço e porta informados, retornando o socket_id */
  socket_id = connect_server(host, port);

  if(socket_id < 0){
    printf("Erro. Não foi possível conectar ao servidor.\n");
    exit(0);
  }

  /* inserindo SSL no socket */
  insertSSLIntoSocketCmd(socket_id);

  /* Conecta ao servidor com o endereço e porta informados, retornando o sync_socket */
  sync_socket = connect_server(host, port);

  if(sync_socket < 0){
    printf("Erro. Não foi possível conectar ao servidor.\n");
    exit(0);
  }

  /* inserindo SSL no socket */
  insertSSLIntoSocketSync(sync_socket);

  int user_auth = auth(ssl_cmd, userid);
  int sync_dir_checked = check_sync_dir();

  /* Se o usuário está OK, então pode executar ações */
  if(user_auth == SUCESSO && sync_dir_checked == SUCESSO){

      /* Aloca dinamicamente para armazenar o número do socket e passar para a thread */
    	//int *arg = malloc(sizeof(*arg));
    	//*arg = sync_socket;
        /* Aloca dinamicamente para armazenar o número do socket e passar para a thread */
        struct arg_struct *args = malloc(sizeof(arg_struct *));
        args->socket_id = socket_id;
        args->sync_socket = sync_socket;

    /* Cria a thread de sincronização */
    if(pthread_create( &s_thread, NULL, sync_thread, (void *)args) != 0){
      printf("[main] ERROR on thread creation.\n");
      SSL_shutdown(ssl_sync);
      SSL_free(ssl_sync);
      close(sync_socket);
      exit(1);
    }

    char command[10];
    char fileName[100];
    char line[110];

    //Atualiza pasta do usuário
    strcpy(sync_dir, "sync_dir_");
    strcat(sync_dir, userid);
    printf("Seu diretório sincronizado é [%s]\n",sync_dir);
    //Monta a lista inicial de arquivos do diretório
    current_files = (file_node*) fn_create_from_path_server_time(sync_dir, socket_id);
    fn_print(current_files);
    char buffer[256];
    printf("\n**************************************\n");
    printf("    Digite seu comando no formato: \n\tupload <filename.ext> \n\tdownload <filename.ext> \n\tlist \n\texit\n");
    printf("**************************************\n");
    while(1){

      bzero(line, 110);
      bzero(buffer, 256);
      bzero(command, 10);
      bzero(fileName,100);

      printf("\n\e[33m%s@dropbox> \e[39m", userid);
      fgets(line, 256,stdin);
      line[strlen(line) - 1] = '\0';

      int i=0;
      char *p, *save;
      for (p = strtok_r(line," ", &save); p != NULL; p = strtok_r(NULL, " ", &save)){
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
        send_file(fileName, buffer, ssl_cmd);
      }
      else if( strcmp("download", command) == 0){
        if (SSL_write(ssl_cmd, buffer, strlen(buffer)) < 0){
            printf("ERROR writing from socket");
        }
        get_file(fileName, ssl_cmd);
      }
      else if( strcmp("list", command) == 0){
        list(buffer, ssl_cmd);
      }
      else if (strcmp("time", command) == 0){
          get_timestamp_server(socket_id);
      }
      else if( strcmp("exit", command) == 0){
        close_connection(buffer, ssl_cmd);
        break;
      }
      else{
        printf("Comando inválido.\n");
      }
    }
  }

  /* Encerra os sockets */
  SSL_shutdown(ssl_cmd);
  close(socket_id);
  SSL_free(ssl_cmd);

  SSL_shutdown(ssl_sync);
  close(sync_socket);
  SSL_free(ssl_sync);

  return 0;
}
