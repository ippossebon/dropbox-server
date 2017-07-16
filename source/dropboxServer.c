#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <dirent.h>
#include <time.h>

#include "../include/dropboxServer.h"
#include "../include/dropboxUtil.h"

/* Temos que conferir se não precisamos definir a porta de maneira mais dinâmica */
#define PORT 4000

/* Globais */
client_node* clients_list;
int num_clients;
pthread_mutex_t lock_num_clients;
pthread_mutex_t mutex_clients_list;
pthread_mutex_t mutex_devices;
pthread_mutex_t mutex_client_files;

/* Recebe as modificações que foram feitas localmente pelo cliente */
void sync_client(SSL *ssl, char* userid){
    char buffer[BUF_SIZE];

    while(1){
        bzero(buffer, BUF_SIZE);
        SSL_read(ssl, buffer, BUF_SIZE);
        if(strcmp(buffer, "end client sync")== 0){
            break;
        }
        char *save;
        char *command = strtok_r(buffer,"#", &save); //upload
        char* file_name = strtok_r(NULL, "#", &save);

        if(strcmp(command, "delete")==0){
            deleteLocalFile(file_name, userid);
        }
        else if(strcmp(command, "upload")==0){
            char file_data[BUF_SIZE];
            bzero(file_data, BUF_SIZE);
            SSL_read(ssl, file_data, BUF_SIZE);
            receive_file(file_name, file_data, userid);
        }
    }

}

void sync_server(SSL *ssl, char* userid){

    char buffer[BUF_SIZE];
    bzero(buffer, BUF_SIZE);
    char* full_path = getClientFolderName(userid);

    /* real_current_files corresponde aos arquivos que estão na pasta do cliente,
    no servidor, no momento atual. Estes arquivos serão comparados com os outros,
    para verificar possíveis modificações. */
    pthread_mutex_lock(&mutex_client_files);
    file_node* real_current_files = fn_create_from_path(full_path);
    pthread_mutex_unlock(&mutex_client_files);

    SSL_read(ssl, buffer, BUF_SIZE);

    char *save;
    char* file_name = strtok_r(buffer, "#", &save);
    char buffer2[BUF_SIZE];

    while(file_name != NULL){

        char* date_file = strtok_r(NULL, "#", &save);
        file_info* file = fn_find(real_current_files, file_name);

        if(file != NULL){ //significa que encontrou um arquivo com mesmo nome

            file->size = -1; // GAMBI =P marcando os arquivos que o cliente tem no server...
            /* s1 < s2 = numero negativo, entao s2 foi atualizada */
            if(strcmp(date_file, file->last_modified) < 0){
                //servidor esta mais atualizado -> ENVIAR PARA CLIENTE
                printf("Enviar arquivo %s modificado para cliente. client:%s server:%s\n", file_name, date_file, file->last_modified);

                bzero(buffer2, BUF_SIZE);
                sprintf(buffer2, "upload#%s", file_name);
                SSL_write(ssl, buffer2, BUF_SIZE);

                send_file(file_name, ssl, userid);
            }

        }else{ // não encontrou, então deve ser deletado no cliente
            //DELETE NO CLIENTE
            printf("DELETAR arquivo %s no cliente. \n", file_name);
            bzero(buffer2, BUF_SIZE);
            sprintf(buffer2, "delete#%s", file_name);
            SSL_write(ssl, buffer2, BUF_SIZE);
        }

        file_name = strtok_r(NULL, "#", &save);
    }

    /* Iterando sobre a lista do server para encontrar os arquivos que não estão no cliente e devem ser adicionados */
    file_node* node;
    for (node = real_current_files; node != NULL; node = node->next) {
        if(node->data->size > -1){
            //ENVIO PARA CLIENTE
            printf("Enviar arquivo %s NOVO para cliente. \n", node->data->name);
            bzero(buffer2, BUF_SIZE);
            sprintf(buffer2, "upload#%s", node->data->name);
            SSL_write(ssl, buffer2, BUF_SIZE);
            send_file(node->data->name, ssl, userid);
        }
    }

  /* Avisa o cliente que terminou de fazer o seu sync. */
  bzero(buffer, BUF_SIZE);
  strcat(buffer, "end server sync");
  SSL_write(ssl, buffer, BUF_SIZE);
}

/* Verifica se existe o diretório sync_dir_user NO DISPOSITIVO DO CLIENTE.
Retorna 0 se não existir; 1, caso contrário.
Já deve criar o outro socket para fazer as operações de sync?*/
int existsSyncFolderClient(char* path){
    return 0;
}

/* Recebe um arquivo file do cliente.
Deverá ser executada quando for realizar upload de um arquivo.
file – path/filename.ext do arquivo a ser recebido */
void receive_file(char *file_name, char* file_data, char *userid){
  /* file_data contém o conteúdo do arquivo a ser enviado para o servidor, e
	filename é o nome do arquivo que está sendo enviado. */
	char *full_path = getClientFolderName(userid);
	strcat(full_path, file_name);

	/* Cria um novo arquivo, na pasta do cliente logado, com o nome do arquivo
	informado, com o conteúdo do arquivo enviado pelo socket. */
	writeBufferToFile(full_path, file_data);
}

/* Envia o arquivo file para o usuário.
Deverá ser executada quando for realizar download de um arquivo.
file – filename.ext */
void send_file(char *file_name, SSL *ssl, char *userid){
    char* full_path = getClientFolderName(userid);
    strcat(full_path, file_name);

    char file_data[BUF_SIZE];
    bzero(file_data, BUF_SIZE);
    writeFileToBuffer(full_path, file_data);

	int n;
	n = SSL_write(ssl, file_data, BUF_SIZE);
	if (n < 0){
		printf("[send_file] Erro ao escrever no socket\n");
	}
}

/* Lista todos os arquivos contidos no diretório remoto do cliente. */
void list(SSL *ssl, char *userid){
    char* full_path = getClientFolderName(userid);
    char buffer[BUF_SIZE];
    int numb_files = 0; /* Gambiarra para verificar se o diretório é vazio*/
    bzero(buffer, BUF_SIZE);

  DIR *d;
  struct dirent *dir;
  d = opendir(full_path);

  if (d){
    /* Itera em todos os arquivo da pasta do cliente e coloca o nome dos
        arquivos dentro do buffer separados por '#' ex: file1#file2#file3# */
        while ((dir = readdir(d)) != NULL){
          numb_files++;
          if (dir->d_type == DT_REG){
              printf("dir->d_name: %s\n", dir->d_name);
            strcat(buffer, dir->d_name);
            strcat(buffer, "#");
          }
        }
    closedir(d);
  }

  if (numb_files <= 2){
      /* O diretório é vazio (um diretório nunca é vazio, ele possui sempre dois
        diretórios: . e .., por isso, verificamos se possui apenas esses dois itens) */
      strcpy(buffer, "empty");
  }

  int n;
  int size = strlen(buffer);

  /* Envia para o client os nomes dos arquivos */
	n = SSL_write(ssl, buffer, BUF_SIZE);
	if (n < 0){
		printf("[list] Erro ao escrever no socket\n");
	}
}

void deleteLocalFile(char* file_name, char* userid){
  char file_path[BUF_SIZE];
  strcpy(file_path, "./client_folders/");
  strcat(file_path, userid);
  strcat(file_path, "/");
  strcat(file_path, file_name);

  int return_value = remove(file_path);
  if (return_value != 0 ){
    printf("Erro ao apagar arquivo %s\n", file_path);
    exit(1);
  }
}

int auth(SSL *ssl, char* userid){
  char buffer[BUF_SIZE];
  bzero(buffer, BUF_SIZE);

  int num_bytes_read, num_bytes_sent;

  /* Mutex para testar se pode conectar mais clientes */
  pthread_mutex_lock(&lock_num_clients);
      if(num_clients >= MAXCLIENTS){
          printf("[auth] Número de clientes conectados excedido\n");
          num_bytes_sent = SSL_write(ssl, "NOT OK", BUF_SIZE);
          printf("[auth] Número de clientes conectados: %d\n", num_clients);
          pthread_mutex_unlock(&lock_num_clients);
          return ERRO;
      }
      else{
          num_clients++;
          printf("[auth] Número de clientes conectados: %d\n", num_clients);
      }
  pthread_mutex_unlock(&lock_num_clients);

  /* No buffer vem o userid do cliente que esta tentando conectar.
  Lê do socket: userid */
  bzero(buffer, BUF_SIZE);
  num_bytes_read = SSL_read(ssl, buffer, BUF_SIZE);
  if (num_bytes_read < 0){
    printf("[auth] ERROR reading from socket \n");
    return ERRO;
  }

  strcpy(userid, buffer);

	/* Verifica se o usuário em questão já é cadastrado ou se é um novo usuário.
	Se for um novo usuário, cria uma struct para ele e adiciona-o a lista de clientes.
	Caso contrário, verifica se o número de dispositivos logados é respeitado.
	Se tudo estiver ok, envia mensagem de sucesso para o cliente e prossegue com
	as operações. */
    pthread_mutex_lock(&mutex_clients_list);
	client* user = findUserInClientsList(clients_list, userid);
    pthread_mutex_unlock(&mutex_clients_list);

	if (user == NULL){

		/* O usuário ainda não é cadastrado. */
		client* new_client = malloc(sizeof(client));
		strcpy(new_client->userid, userid);

        char dir_client[128];
        bzero(dir_client, 128);
        strcat(dir_client, "./client_folders/");
        strcat(dir_client, new_client->userid);

        if(existsFolder(dir_client) == 0){
            /* cria uma pasta para o usuário no server */
            char dir[100];
            strcpy(dir, "mkdir ");
            strcat(dir, dir_client);
            system(dir);
        }

        pthread_mutex_lock(&mutex_devices);
        new_client->devices[0] = 1;
		new_client->devices[1] = 0;
        pthread_mutex_unlock(&mutex_devices);

        /* fn_create_from_path() inicializa uma lista com os arquivos contidos
        na pasta deste cliente no servidor. */
        pthread_mutex_lock(&mutex_client_files);
        new_client->files = fn_create_from_path(dir_client);
        pthread_mutex_unlock(&mutex_client_files);

        new_client->logged_in = 1;

        pthread_mutex_lock(&mutex_clients_list);
		clients_list = addClientToList(clients_list, new_client);
        pthread_mutex_unlock(&mutex_clients_list);

		printf("[auth] Novo usuário > adicionado à lista. Lista de clientes: \n");
        printClientsList(clients_list);
	}
	else{
        pthread_mutex_lock(&mutex_devices);

        /* Se o cliente já está cadastrado no sistema, verifica o número de
		dispositivos nos quais está logado. */
		if (user->devices[0] == 1 && user->devices[1] == 1){
			printf("ERRO: Este cliente já está logado em dois dispositivos diferentes.\n");
            num_bytes_sent = SSL_write(ssl, "NOT OK", BUF_SIZE);
			return ERRO;
		}
        else if (user->devices[0] == 0){
            user->devices[0] = 1;
        }
        else if (user->devices[1] == 0){
            user->devices[1] = 1;
        }

		printf("Usuário liberado.\n");
        pthread_mutex_unlock(&mutex_devices);
	}

    num_bytes_sent = SSL_write(ssl, "OK", BUF_SIZE);

	if (num_bytes_sent < 0){
		printf("[auth] ERROR writing on socket\n");
        return ERRO;
	}
    return SUCESSO;
}

void receive_command_client(SSL *ssl, char *userid){
  char buffer[BUF_SIZE];
  char command[10];
  char file_name[32];
  char file_data[BUF_SIZE];
  int num_bytes_read;

  while(1){
        bzero(buffer, BUF_SIZE);
        bzero(file_data, BUF_SIZE);
	    char *p;
	    int i = 0;

        printf("Userid: %s\n", userid);
        printClientsList(clients_list);

	    /* Recebe
        DOWNLOAD: download#filename
        UPLOAD: upload#filename#conteudo_do_arquivo
        LIST: list#
        GET_SYNC_DIR: get_sync_dir#
        TIME: time#
        EXIT: exit# */
	    num_bytes_read = SSL_read(ssl, buffer, BUF_SIZE);

	    if (num_bytes_read < 0){
            printf("[receive_command_client] Erro ao ler linha de comando do socket.\n");
            printf("BUFFER: %s\n", buffer);
            exit(1);
        }

	    /* Separa o buffer de acordo com as informações necessárias, onde o delimitador é #.*/
        char *save;
	    for (p = strtok_r(buffer,"#", &save); p != NULL; p = strtok_r(NULL, "#", &save)){
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
        if(strcmp("upload", command) == 0){
            receive_file(file_name, file_data, userid);
        }
        else if(strcmp("download", command) == 0){
            send_file(file_name, ssl, userid);
        }
        else if(strcmp("list", command) == 0){
            list(ssl, userid);
        }
        else if (strcmp("delete", command) == 0){
            //deleteLocalFile(file_name);
        }
        else if (strcmp("time", command) == 0){
            send_time(ssl);
        }
        else if(strcmp("exit", command) == 0){
            close_connection(userid);
            return;
        }
  }
}


void *sync_thread(void *args_sync){
    /* Uns casts muito loucos */
    arg_struct_sync *args = args_sync;
    SSL *ssl_sync = args->ssl_sync;
    char userid[MAXNAME];
    strcpy(userid, args->userid);

    /*
    A sincronização é feita em duas etapas:
    1. sync_client: O cliente verifica todas as modificações realizadas localemente
    e informa ao servidor quais são os arquivos que devem ser enviados ou apagados.
    2. sync_server: O servidor faz o mesmo passo acima.
    */
    char buffer[BUF_SIZE];

     while(1){
        bzero(buffer, BUF_SIZE);
        SSL_read(ssl_sync, buffer, BUF_SIZE);

        if(strcmp(buffer, "start client sync") == 0){
            sync_client(ssl_sync, userid);
            sync_server(ssl_sync, userid);
        }
     }
     return 0;
}

void *client_thread(void *new_sockets){
	/* Uns casts muito loucos */
    arg_struct *args = new_sockets;
	int socket_id = args->socket_id;
    int sync_socket = args->sync_socket;
    SSL *ssl_sync = args->ssl_sync;
    SSL *ssl_cmd = args->ssl_cmd;

    char userid[MAXNAME];
    pthread_t s_thread;

	/* Faz verificação de usuário, volta com o nome do cliente em userid */
	if (auth(ssl_cmd, userid) == ERRO){
        SSL_shutdown(ssl_cmd);
        close(socket_id);
        SSL_free(ssl_cmd);

        SSL_shutdown(ssl_sync);
        close(sync_socket);
        SSL_free(ssl_sync);

        free(new_sockets);
        printf("[client_thread] Erro ao autenticar o usuário\n");
        pthread_exit(NULL);
    }

    /* Aloca dinamicamente para armazenar o número do socket e passar para a thread de sync */
    arg_struct_sync *args_sync = malloc(sizeof(arg_struct_sync *));
    args_sync->sync_socket = sync_socket;
    args_sync->ssl_sync = ssl_sync;
    strcpy(args_sync->userid, userid);

    /* Cria a thread de sincronização para aquele cliente */
    if(pthread_create(&s_thread, NULL, sync_thread, (void *) args_sync) != 0){
        printf("[client_thread] ERROR on thread creation.\n");
        pthread_exit(NULL);
    }

	/* Recebe a linha de comando e redireciona para a função objetivo */
	receive_command_client(ssl_cmd, userid);

    /* Mata a thread de sincronização */
    pthread_cancel(s_thread);

    SSL_shutdown(ssl_cmd);
	close(socket_id);
    SSL_free(ssl_cmd);

    SSL_shutdown(ssl_sync);
    close(sync_socket);
    SSL_free(ssl_sync);
    free(new_sockets);

	return 0;
}

void send_time(SSL *ssl){
    time_t now;
    struct tm *local_time;
    char timestamp[BUF_SIZE];
    bzero(timestamp, BUF_SIZE);

    now = time (NULL);
    local_time = localtime (&now);

    /* Coloca a data no formato: aaaa.mm.dd hh:mm:ss */
    char seconds[2];
    sprintf(seconds, "%d", local_time->tm_sec);
    char minutes[2];
    sprintf(minutes, "%d", local_time->tm_min);
    char hour[2];
    sprintf(hour, "%d", local_time->tm_hour);
    char day[2];
    sprintf(day, "%d", local_time->tm_mday);
    char month[2];
    sprintf(month, "%d", local_time->tm_mon);
    char year[2];
    sprintf(year, "%d", local_time->tm_year + 1900);

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
    printf("timestamp %s\n", timestamp);

    int n;
    n = SSL_write(ssl, timestamp, BUF_SIZE);
    printf("Vai escrever o seguinte timestamp no socket: %s\n", timestamp);
    if (n < 0){
        printf("[sendTime] Erro ao escrever timestamp no socket\n");
    }
}


int main(int argc, char *argv[]){
	int server_socket_id, new_socket_id, new_sync_socket;
    struct sockaddr_in server_address, client_address, client_sync_address;
	socklen_t client_len;

    /* Inicializando o SSL */
    initializeSSL();
    printf("[main] Inicializou SSL.\n");

    /* SSL Sync */
    SSL_METHOD *method_sync;
    SSL_CTX *ctx_sync; //ponteiro para a estrutura do contexto do sync
    SSL *ssl_sync;

    /* SSL Cmd */
    SSL_METHOD *method_cmd;
    SSL_CTX *ctx_cmd; //ponteiro para a estrutura do contexto dos comandos
    SSL *ssl_cmd;

    //transformar em funções depois
    method_cmd = SSLv23_server_method();
    ctx_cmd = SSL_CTX_new(method_cmd);
    if (ctx_cmd == NULL) {
        ERR_print_errors_fp(stderr);
        abort();
    }

    //transformar em funções depois
    method_sync = SSLv23_server_method();
    ctx_sync = SSL_CTX_new(method_sync);
    if (ctx_sync == NULL) {
        ERR_print_errors_fp(stderr);
        abort();
    }

    /* SSL carrega certificados */
    SSL_CTX_use_certificate_file(ctx_sync, "CertFile.pem", SSL_FILETYPE_PEM);
    SSL_CTX_use_PrivateKey_file(ctx_sync, "KeyFile.pem", SSL_FILETYPE_PEM);
    SSL_CTX_use_certificate_file(ctx_cmd, "CertFile.pem", SSL_FILETYPE_PEM);
    SSL_CTX_use_PrivateKey_file(ctx_cmd, "KeyFile.pem", SSL_FILETYPE_PEM);

    /* Inicializa mutex */
    if (pthread_mutex_init(&mutex_devices, NULL) != 0){
        printf("[main] ERRO na inicialização do mutex_devices\n");
        exit(1);
    }

    if (pthread_mutex_init(&lock_num_clients, NULL) != 0){
        printf("[main] ERRO na inicialização do lock_num_clients\n");
        exit(1);
    }

    if (pthread_mutex_init(&mutex_clients_list, NULL) != 0){
        printf("[main] ERRO na inicialização do mutex_clients_list\n");
        exit(1);
    }

    if (pthread_mutex_init(&mutex_client_files, NULL) != 0){
        printf("[main] ERRO na inicialização do mutex_client_files\n");
        exit(1);
    }

	pthread_t c_thread;

    /* Inicializa uma lista de clientes. */
    pthread_mutex_lock(&mutex_clients_list);
	clients_list = createClientsList();
    pthread_mutex_unlock(&mutex_clients_list);

    /* Cria socket TCP para o servidor. */
	if ((server_socket_id = socket(AF_INET, SOCK_STREAM, 0)) == -1){
		printf("[main] ERROR opening socket\n");
        exit(1);
	}

	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(PORT);
	server_address.sin_addr.s_addr = INADDR_ANY;
	bzero(&(server_address.sin_zero), 8);

	if(bind(server_socket_id, (struct sockaddr *) &server_address, sizeof(server_address)) < 0){
		printf("[main] ERROR on binding\n");
        exit(1);
	}

    /* Informa que o socket em questões pode receber conexões, 5 indica o
    tamanho da fila de mensagens */
	listen(server_socket_id, 10);

	client_len = sizeof(struct sockaddr_in);

    /* Zero clientes conectados */
    num_clients = 0;
    printf("[main] Before while\n");
	/* Laço que fica aguardando conexões de clientes e criandos as threads*/
	while(1){

	    /* Aguarda a conexão do cliente no socket principal */
		if((new_socket_id = accept(server_socket_id, (struct sockaddr *) &client_address, &client_len)) != ERRO){
            printf("[main] entrou no accept\n");
            /*
                SSL Handshake
                Socket de comando
            */
            ssl_cmd	= SSL_new(ctx_cmd);
            SSL_set_fd(ssl_cmd,	new_socket_id);
            int ssl_err = SSL_accept(ssl_cmd);
            if(ssl_err <= 0)
            {
                //Erro aconteceu, fecha o SSL
                printf("[main] Erro com SSL\n");
                exit(1);
            }

            /* Aguarda a conexão do cliente no socket de sincronização */
            if((new_sync_socket = accept(server_socket_id, (struct sockaddr *) &client_sync_address, &client_len)) != ERRO){

                /*
                    SSL Handshake
                    Socket de Sync
                */
                ssl_sync = SSL_new(ctx_sync);
                SSL_set_fd(ssl_sync, new_sync_socket);
                int ssl_err = SSL_accept(ssl_sync);
                if(ssl_err <= 0)
                {
                    //Erro aconteceu, fecha o SSL
                    printf("[main] Erro com SSL\n");
                    exit(1);
                }

                /* Aloca dinamicamente para armazenar o número do socket e passar para a thread */
                arg_struct *args = malloc(sizeof(arg_struct *));
                args->socket_id = new_socket_id;
                args->sync_socket = new_sync_socket;
                args->ssl_sync = ssl_sync; //modificado para ter os respectivos ssl's
                args->ssl_cmd = ssl_cmd; //modificado para ter os respectivos ssl's

            	/* Se conectou, cria a thread para o cliente, enviando o id do socket */
            	if(pthread_create( &c_thread, NULL, client_thread, (void *)args) != 0){
            		printf("[main] ERROR on thread creation.\n");
                    SSL_shutdown(ssl_cmd);
            		close(new_socket_id);
                    SSL_free(ssl_cmd);

                    SSL_shutdown(ssl_sync);
                    close(new_sync_socket);
                    SSL_free(ssl_sync);
                    exit(1);
                }
            }
            else{
                printf("[main] Erro no accept do sync\n");
                SSL_shutdown(ssl_sync);
                close(new_sync_socket);
                SSL_free(ssl_sync);
                exit(1);
            }
        }
        else{
            printf("[main] Erro no accept\n");
            SSL_shutdown(ssl_cmd);
            close(new_socket_id);
            SSL_free(ssl_cmd);
            exit(1);
        }
    }

	close(server_socket_id);

    pthread_mutex_lock(&mutex_clients_list);
	clients_list = clearClientsList(clients_list);
    pthread_mutex_lock(&mutex_clients_list);

    pthread_mutex_destroy(&lock_num_clients);
    pthread_mutex_destroy(&mutex_devices);
    pthread_mutex_destroy(&mutex_clients_list);
    pthread_mutex_destroy(&mutex_client_files);

	return 0;
}

void close_connection(char* userid){
    printf("[close_connection] Encerrando a conexão do cliente %s\n", userid);

    /* Decrementa o número de clientes conectados */
    pthread_mutex_lock(&lock_num_clients);
    num_clients--;
    printf("[close_connection] Número de clientes conectados: %d\n", num_clients);
    pthread_mutex_unlock(&lock_num_clients);

    pthread_mutex_lock(&mutex_clients_list);
    client* user = findUserInClientsList(clients_list, userid);
    pthread_mutex_unlock(&mutex_clients_list);

    if (user == NULL){
        printf("[close_connection] Erro: usuário não encontrado\n");
        pthread_exit(NULL);
    }

    pthread_mutex_lock(&mutex_devices);

    /* Desloga de um dos dispositivos. */
    if (user->devices[1] == 1){
        user->devices[1] = 0;
    }
    else if (user->devices[0] == 1){
        user->devices[0] = 0;
    }

    /*Se não estiver logado em nenhum outro dispositivo, retira o usuário da lista.*/
    if (user->devices[0] == 0 && user->devices[1] == 0){
        pthread_mutex_lock(&mutex_clients_list);
        clients_list = removeClientFromList(clients_list, userid);
        pthread_mutex_unlock(&mutex_clients_list);
    }

    pthread_mutex_unlock(&mutex_devices);
}
