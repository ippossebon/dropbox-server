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

#include "../include/dropboxServer.h"
#include "../include/dropboxUtil.h"

/* Temos que conferir se não precisamos definir a porta de maneira mais dinâmica */
#define PORT 4000

/* Globais */
client_node* clients_list;
int num_clients;
pthread_mutex_t lock_num_clients;

/* Recebe as modificações que foram feitas localmente pelo cliente */
void sync_client(int sync_socket, char* userid){
    //printf("No sync client, esperando o cliente começar. sync_socket: %d, userid: %s\n", sync_socket, userid);

    char buffer[BUF_SIZE];

    while(1){
        bzero(buffer, BUF_SIZE);
        read(sync_socket, buffer, BUF_SIZE);
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
            read(sync_socket, file_data, BUF_SIZE);
            receive_file(file_name, file_data, userid);
        }
    }

}

void sync_server(int sync_socket, char* userid){

    char buffer[BUF_SIZE];
    bzero(buffer, BUF_SIZE);
    char* full_path = getClientFolderName(userid);
    file_node* real_current_files = fn_create_from_path(full_path);

    read(sync_socket, buffer, BUF_SIZE);

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
                write(sync_socket, buffer2, BUF_SIZE);

                send_file(file_name, sync_socket, userid);
            }

        }else{ // não encontrou, então deve ser deletado no cliente
            //DELETE NO CLIENTE
            printf("DELETAR arquivo %s no cliente. \n", file_name);
            bzero(buffer2, BUF_SIZE);
            sprintf(buffer2, "delete#%s", file_name);
            write(sync_socket, buffer2, BUF_SIZE);

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
            write(sync_socket, buffer2, BUF_SIZE);
            send_file(node->data->name, sync_socket, userid);
        }
    }

  /* Avisa o cliente que terminou de fazer o seu sync. */
  bzero(buffer, BUF_SIZE);
  strcat(buffer, "end server sync");
  write(sync_socket, buffer, BUF_SIZE);
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

    //printf("Folder do cliente: %s\n", full_path);
	/* Cria um novo arquivo, na pasta do cliente logado, com o nome do arquivo
	informado, com o conteúdo do arquivo enviado pelo socket. */
	writeBufferToFile(full_path, file_data);
}

/* Envia o arquivo file para o usuário.
Deverá ser executada quando for realizar download de um arquivo.
file – filename.ext */
void send_file(char *file_name, int socket, char *userid){
    char* full_path = getClientFolderName(userid);
    strcat(full_path, file_name);

    char file_data[BUF_SIZE];
    bzero(file_data, BUF_SIZE);
    writeFileToBuffer(full_path, file_data);

	int n;
	n = write(socket, file_data, BUF_SIZE);
	if (n < 0){
		printf("[send_file] Erro ao escrever no socket\n");
	}
}

/* Lista todos os arquivos contidos no diretório remoto do cliente. */
void list(int socket, char *userid){

  char* full_path = getClientFolderName(userid);
  char buffer[256];
  int numb_files = 0; /* Gambiarra para verificar se o diretório é vazio*/
  bzero(buffer, 256);

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
	n = write(socket, buffer, size);
	if (n < 0){
		printf("[list] Erro ao escrever no socket\n");
	}
}

void deleteLocalFile(char* file_name, char* userid){
  char file_path[256];
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

int auth(int socket, char* userid){
  char buffer[256];
  bzero(buffer, 256);

  int num_bytes_read, num_bytes_sent;

  /* No buffer vem o userid do cliente que esta tentando conectar.
  Lê do socket: userid */
  num_bytes_read = read(socket, buffer, 256);
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
	client* user = findUserInClientsList(clients_list, userid);

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

        /* TODO: como inicializar a variavel devices? */
        new_client->devices[0] = 1;
		new_client->devices[1] = 0;
        new_client->files = fn_create_from_path(dir_client);
		new_client->logged_in = 1;

		clients_list = addClientToList(clients_list, new_client);
		printf("[auth] Novo usuário > adicionado à lista. Lista de clientes: \n");

        printClientsList(clients_list);
	}
	else{
        /*TODO: SEÇÃO CRÍTICA*/

        /* Se o cliente já está cadastrado no sistema, verifica o número de
		dispositivos nos quais está logado. */
		if (user->devices[0] == 1 && user->devices[1] == 1){
			printf("ERRO: Este cliente já está logado em dois dispositivos diferentes.\n");
            num_bytes_sent = write(socket, "NOT OK", 7);
			return ERRO;
		}
        else if (user->devices[0] == 0){
            user->devices[0] = 1;
        }
        else if (user->devices[1] == 0){
            user->devices[1] = 1;
        }

		printf("Usuário liberado.\n");
	}

    num_bytes_sent = write(socket, "OK", 3);

	if (num_bytes_sent < 0){
		printf("[auth] ERROR writing on socket\n");
        return ERRO;
	}
    return SUCESSO;
}

void receive_command_client(int socket, char *userid){
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
        EXIT: exit# */
	    num_bytes_read = read(socket, buffer, BUF_SIZE);

	    if (num_bytes_read < 0){
            printf("[receive_command_client] Erro ao ler linha de comando do socket.\n");
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
            send_file(file_name, socket, userid);
        }
        else if(strcmp("list", command) == 0){
            list(socket, userid);
        }
        else if (strcmp("delete", command) == 0){
            //deleteLocalFile(file_name);
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
    int sync_socket = args->sync_socket;
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
        read(sync_socket, buffer, BUF_SIZE);

        if(strcmp(buffer, "start client sync") == 0){
            sync_client(sync_socket, userid);
            sync_server(sync_socket, userid);
        }
     }
     return 0;
}

void *client_thread(void *new_sockets){
	/* Uns casts muito loucos */
    arg_struct *args = new_sockets;
	int socket_id = args->socket_id;
    int sync_socket = args->sync_socket;
    char userid[MAXNAME];
    pthread_t s_thread;

	/* Faz verificação de usuário, volta com o nome do cliente em userid */
	if (auth(socket_id, userid) == ERRO){
        close(socket_id);
        close(sync_socket);
        free(new_sockets);
        printf("[client_thread] Erro ao autenticar o usuário\n");
        pthread_exit(NULL);
    }

    /* Aloca dinamicamente para armazenar o número do socket e passar para a thread de sync */
    arg_struct_sync *args_sync = malloc(sizeof(arg_struct_sync *));
    args_sync->sync_socket = sync_socket;
    strcpy(args_sync->userid, userid);

    /* Cria a thread de sincronização para aquele cliente */
    if(pthread_create(&s_thread, NULL, sync_thread, (void *) args_sync) != 0){
        printf("[client_thread] ERROR on thread creation.\n");
        pthread_exit(NULL);
    }

	/* Recebe a linha de comando e redireciona para a função objetivo */
	receive_command_client(socket_id, userid);

    /* Mata a thread de sincronização */
    pthread_cancel(s_thread);

	close(socket_id);
    close(sync_socket);
    free(new_sockets);

	return 0;
}


int main(int argc, char *argv[]){
	int server_socket_id, new_socket_id, new_sync_socket;
    struct sockaddr_in server_address, client_address, client_sync_address;
	socklen_t client_len;

    if (pthread_mutex_init(&lock_num_clients, NULL) != 0){
        printf("[main] ERROR criando mutex.\n");
        exit(1);
    }

    /* Talvez devêssemos alocar por malloc
    TODO: rever */
	pthread_t c_thread;

    /* Inicializa uma lista de clientes. */
	clients_list = createClientsList();

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
    /*TODO: por que está aceitando mais do que 5 clientes?*/
    /* Dez conexões: 5 pro socket principal e 5 pra sincronização */
	listen(server_socket_id, 10);

	client_len = sizeof(struct sockaddr_in);

    /* Zero clientes conectados */
    num_clients = 0;

	/* Laço que fica aguardando conexões de clientes e criandos as threads*/
	while(1){

        printf("[main] Número de clientes: %d\n", num_clients);

        int ctrl;
        /* Mutex para testar se pode conectar mais clientes */
        pthread_mutex_lock(&lock_num_clients);
            if(num_clients >= MAXCLIENTS){
                ctrl = ERRO;
            }
            else{
                num_clients++;
                ctrl = SUCESSO;
            }
        pthread_mutex_unlock(&lock_num_clients);

        if(ctrl == SUCESSO){
    	    /* Aguarda a conexão do cliente no socket principal */
    		if((new_socket_id = accept(server_socket_id, (struct sockaddr *) &client_address, &client_len)) != ERRO){

                /* Aguarda a conexão do cliente no socket de sincronização */
                if((new_sync_socket = accept(server_socket_id, (struct sockaddr *) &client_sync_address, &client_len)) != ERRO){

                    /* Aloca dinamicamente para armazenar o número do socket e passar para a thread */
                    arg_struct *args = malloc(sizeof(arg_struct *));
                    args->socket_id = new_socket_id;
                    args->sync_socket = new_sync_socket;

                	/* Se conectou, cria a thread para o cliente, enviando o id do socket */
                	if(pthread_create( &c_thread, NULL, client_thread, (void *)args) != 0){
                		printf("[main] ERROR on thread creation.\n");
                		close(new_socket_id);
                        close(new_sync_socket);
                        exit(1);
                    }
                }
                else{
                    printf("[main] Erro no accept do sync\n");
                    close(new_sync_socket);
                    exit(1);
                }
            }
            else{
                printf("[main] Erro no accept\n");
                close(new_socket_id);
                exit(1);
            }
        }
    }

	close(server_socket_id);

	//clients_list = clearClientsList(clients_list);
    pthread_mutex_destroy(&lock_num_clients);

	return 0;
}

void close_connection(char* userid){
    printf("[close_connection] Encerrando a conexão do cliente %s\n", userid);

    /* Decrementa o número de clientes conectados */
    pthread_mutex_lock(&lock_num_clients);
        num_clients--;
    pthread_mutex_unlock(&lock_num_clients);

    client* user = findUserInClientsList(clients_list, userid);

    if (user == NULL){
        printf("[close_connection] Erro: usuário não encontrado\n");
        pthread_exit(NULL);
    }

    printf("Vai deslogar o usuario: %s\n", user->userid);

    /* Desloga de um dos dispositivos. */
    if (user->devices[1] == 1){
        user->devices[1] = 0;
    }
    else if (user->devices[0] == 1){
        user->devices[0] = 0;
    }

    /*Se não estiver logado em nenhum outro dispositivo, retira o usuário da lista.*/
    if (user->devices[0] == 0 && user->devices[1] == 0){
        user->logged_in = 0;
        clients_list = removeClientFromList(clients_list, userid);
    }

    printf("Terminou de deslogar...\n");
    printClientsList(clients_list);
}
