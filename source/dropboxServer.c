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

client_node* clients_list;

/* Recebe as modificações que foram feitas localmente pelo cliente */
void sync_client(int sync_socket, char* userid){
    printf("No sync client, esperando o cliente começar. sync_socket: %d, userid: %s\n", sync_socket, userid);

    char buffer[256];
    bzero(buffer, 256);
    read(sync_socket, buffer, 256);

    if (strcmp(buffer, "start client sync") == 0){
        printf("Client vai começar a sync\n");

        receive_command_client(sync_socket, userid);
    }
    else{
        printf("[sync_client] Erro ao receber comando de sync do client.\n");
        exit(1);
    }
}

void sync_server(int sync_socket, char* userid){
  /* Avisa o client que vai iniciar o seu sync.*/
  char buffer[256];
  strcat(buffer, "start server sync");
  int buffer_size = strlen(buffer);
  write(sync_socket, buffer, 256);

  /* Aqui estará a nova lista com os arquivos atuais na pasta ./client_folders/userid/*/
  char client_local_dir[256];
  strcat(client_local_dir, "./client_folders/");
  strcat(client_local_dir, userid);

  file_node* real_current_files = fn_create_from_path(client_local_dir);
  struct client* current_client = findUserInClientsList(clients_list, userid);
  file_node* current_files = current_client->files;

  file_node* node;

  /* Para cada arquivo "real/nova" bucamos na lista "antiga" se ele existe.
      Se encontrar: verifica se foi modificado. Se tiver sido modificado então = UPLOAD do arquivo.
      Se não encontrar: significa que esse arquivo foi adicionado = UPLOAD arquivo */
  for (node = real_current_files; node !=NULL; node = node->next) {
    /* Retorna o file_info do nodo se encontrado, se não retorna NULL */
    file_info* old_file = fn_find(current_files, node->data->name);

    if(old_file != NULL){ //encontrou o arquivo, verifica se foi modificado
      /* s1 < s2 = numero negativo, entao s2 foi atualizada */
      if(strcmp(old_file->last_modified, node->data->last_modified) < 0){
        printf("O arquivo %s foi MODIFICADO às %s\n", node->data->name, node->data->last_modified);
        /* Deve enviar o arquivo para o client (análogo a uma chamada
        de DOWNLOAD). */
        bzero(buffer, 256);
        strcat(buffer, "download");
        int buffer_size = strlen(buffer);
        write(sync_socket, buffer, buffer_size);

        send_file(node->data->name, sync_socket, userid);
      }
    }
    else { //Não encontrou o arquivo, então ele foi adicionado
      printf("O arquivo %s foi ADICIONADO às %s\n", node->data->name, node->data->last_modified);

      /* Deve enviar o arquivo para o client (análogo a uma chamada
      de DOWNLOAD). */
      bzero(buffer, 256);
      strcat(buffer, "download");
      int buffer_size = strlen(buffer);
      write(sync_socket, buffer, buffer_size);

      send_file(node->data->name, sync_socket, userid);
    }
  }

  /* Para cada arquivo "antigo" bucamos na lista "real/nova" se ele existe.
    Se encontrar: acredito que a verificação de cima já está fazendo o que precisa ser feito, então não faria nada nesse caso.
    Se não encontrar: significa que esse arquivo foi deletado = DELETE arquivo */
  for (node = current_files; node !=NULL; node = node->next) {
    /* Retorna o file_info do nodo se encontrado, se não retorna NULL */
    file_info* old_file = fn_find(real_current_files, node->data->name);

    if(old_file != NULL){ //encontrou o arquivo, verifica se foi modificado
      /* s1 < s2 = numero negativo, entao s2 foi atualizada */
      /* Se encontrar: acredito que a verificação de cima já está fazendo o que precisa ser feito,
        então não faria nada nesse caso. */
      if(strcmp(old_file->last_modified, node->data->last_modified) < 0){
        printf("O arquivo %s foi MODIFICADO às %s\n", node->data->name, node->data->last_modified);

        /* Deve enviar o arquivo para o client (análogo a uma chamada
        de DOWNLOAD). */
        bzero(buffer, 256);
        strcat(buffer, "download");
        int buffer_size = strlen(buffer);
        write(sync_socket, buffer, buffer_size);

        send_file(node->data->name, sync_socket, userid);
      }
    }
    else{ //Não encontrou o arquivo, então ele foi deletado
      printf("O arquivo %s foi DELETADO.\n", node->data->name);

      /* Deve pedir para o client apagar o arquivo */
      bzero(buffer, 256);
      strcat(buffer, "delete#");
      strcat(buffer, node->data->name);
      buffer_size = strlen(buffer);
      write(sync_socket, buffer, buffer_size);
    }
  }

  fn_clear(current_files);
  /* current_file agora aponta para a real_current_files */
  current_files = real_current_files;

  /* Avisa o servidor que terminou de fazer o seu sync. */
  bzero(buffer, 256);
  strcat(buffer, "end server sync");
  buffer_size = strlen(buffer);
  write(sync_socket, buffer, buffer_size);
}

/* Verifica se existe o diretório sync_dir_user NO DISPOSITIVO DO CLIENTE.
Retorna 0 se não existir; 1, caso contrário.
Já deve criar o outro socket para fazer as operações de sync?*/
int existsSyncFolderClient(char* path){
    return 0;
}

/* O comando get_sync_dir é executado logo após o estabelecimento de uma conexão entre cliente e
servidor. Toda vez que o comando get_sync_dir for executado, o servidor verificará se o diretório
“sync_dir_<nomeusuário>” existe no dispositivo do cliente. Em caso afirmativo, nada deverá ser feito.
Caso contrário, o diretório deverá ser criado e a sincronização ser efetuada pelo cliente */
int get_sync_dir(char* userid){
	char sync_name[255];
	strcat(sync_name, "sync_dir_");
	strcat(sync_name, userid);

  /* Cria um novo socket, responsável pelas operações de sync entre o servidor
  e o cliente. Através desse scoket, envia a mensagem #exists#userid para saber
  se existe uma pasta sync_dir_userid no cliente. Em caso afirmativo, nada deverá
  ser feito. Se a pasta não existir, o servidor deverá criá-la.
  O cliente tem uma pasta sync_dir_userid por device, e a criação dessas pastas
  é uma condição de corrida. */

  // Cria um novo socket para a sincronização
  int sync_socket = createSocket(PORT + 1);

  if (sync_socket == ERRO){
    printf("[get_sync_dir] Erro ao criar sync socket.\n");
    return ERRO;
  }
  char buffer[256];
  strcat(buffer, "exists");

  int num_bytes_sent;
  int size = strlen(buffer);

  /* Envia para o client os nomes dos arquivos */
  num_bytes_sent = write(sync_socket, buffer, size);
  if (num_bytes_sent < 0){
    printf("[get_sync_dir] Erro ao escrever no socket\n");
    return ERRO;
  }

  /* Lê resposta do cliente. */
  bzero(buffer, 256);
  int num_bytes_read = read(sync_socket, buffer, 256);

  if (num_bytes_read < 0){
    printf("[get_sync_dir] ERROR reading from socket \n");
  }

    /* Não existe uma pasta sync_dir_userid no device. Portanto, deve criá-la.
    Se existir, nada deve ser feito. */
    if (strcmp(buffer, "false") == 0){
      printf("O usuário não possuia uma pasta sync_dir neste device. Uma pasta sync_dir foicriada para ele.\n");
    }
    else{
      printf("A pasta sync_dir do cliente foi localizada no device.\n");
    }

  return sync_socket;
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

  char file_data[256];
  bzero(file_data, 256);
	writeFileToBuffer(full_path, file_data);

	int n;
  int size = strlen(file_data);
	n = write(socket, file_data, size);
	if (n < 0){
		printf("[send_file] Erro ao escrever no socket\n");
	}
}

/* Lista todos os arquivos contidos no diretório remoto do cliente. */
void list(int socket, char *userid){

  char* full_path = getClientFolderName(userid);
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
		printf("[list] Erro ao escrever no socket\n");
	}
}

void deleteLocalFile(char* file_name, char* userid){
  char file_path[256];
  strcat(file_path, "./client_folders/");
  strcat(file_path, userid);
  strcat(file_path, "/");
  strcat(file_path, file_name);

  int return_value = remove(file_path);
  if (return_value != 0 ){
    printf("Erro ao apagar arquivo %s\n", file_path);
    exit(1);
  }
}

void auth(int socket, char* userid){
  char buffer[256];
  bzero(buffer, 256);

  int num_bytes_read, num_bytes_sent;

  /* No buffer vem o userid do cliente que esta tentando conectar */
  num_bytes_read = read(socket, buffer, 256);
  if (num_bytes_read < 0){
    printf("[auth] ERROR reading from socket \n");
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
		struct client new_client;
		strcpy(new_client.userid, userid);

    char dir_client[128];
    bzero(dir_client, 128);
    strcat(dir_client, "./client_folders/");
    strcat(dir_client, new_client.userid);

    if(existsFolder(dir_client) == 0){
      /* cria uma pasta para o usuário no server */
      printf("[auth] Vai criar a pasta.\n");
      char dir[100];
      strcpy(dir, "mkdir ");
      strcat(dir, dir_client);
      system(dir);
    }

    printf("[auth] Deve ter criado a pasta.\n");
		/*TODO: como inicializar a variavel devices? */
		new_client.devices[0] = 0;
		new_client.devices[1] = 0;
    new_client.files = fn_create_from_path(dir_client);
		new_client.logged_in = 1;

		clients_list = addClientToList(clients_list, &new_client);
		printf("[auth] Novo usuário > adicionado à lista. Lista de clientes: \n");
		printClientsList(clients_list);
	}
	else{
		/* Se o cliente já está cadastrado no sistema, verifica o número de
		dispositivos nos quais está logado. */
		if (user->devices[0] == 1 && user->devices[1] == 1){
			printf("ERRO: Este cliente já está logado em dois dispositivos diferentes.\n");
			return;
		}

		/* TODO: Como registrar o device em que ele está cadastrado? */

		printf("Usuário liberado.\n");
	}

	/* Envia mensagem de sucesso para o cliente. */
	bzero(buffer, 256);
	num_bytes_sent = write(socket, "OK", 3);

	if (num_bytes_sent < 0){
		printf("[auth] ERROR writing on socket\n");
	}
}

void receive_command_client(int socket, char *userid){
  char buffer[256];
  char command[10];
  char file_name[32];
  int num_bytes_read;

  while(1){
    bzero(buffer, 256);
	    char file_data[256];
	    char *p;
	    int i = 0;

	    /* Recebe comando#filename#arquivo (se existirem) */
	    num_bytes_read = read(socket, buffer, 256);

	    if (num_bytes_read < 0){
            printf("[receive_command_client] Erro ao ler linha de comando do socket.\n");
        }

        printf("Server recebeu: %s\n", buffer);

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
            receive_file(file_name, file_data, userid);
        }
        else if( strcmp("download", command) == 0){
            send_file(file_name, socket, userid);
        }
        else if( strcmp("list", command) == 0){
            list(socket, userid);
        }
        else if (strcmp("delete", command) == 0){
            //deleteLocalFile(file_name);
        }
        else if( strcmp("get_sync_dir", command) == 0){
            //sync_server();
        }
        else if(strcmp("end client sync", command) == 0){
            /* Utilizada para sincronização do client */
            return;
        }
        else if( strcmp("exit", command) == 0){
            return;
        }
  }
}


void *sync_thread(void *userid){
    int sync_socket;

      sync_socket = get_sync_dir((char *) userid);
      if(sync_socket == ERRO){
        printf("[client_thread] Erro no get_sync_dir\n");
        exit(1);
      }

      while(1){
          sync_client(sync_socket, userid);
          sleep(15);
          sync_server(sync_socket, userid);
      }

      close(sync_socket);
      return 0;
}

void *client_thread(void *new_socket_id){
	/* Uns casts muito loucos */
	int socket_id = *((int *) new_socket_id);
  char userid[MAXNAME];
  pthread_t s_thread;

	/* Faz verificação de usuário. TODO: verificar se existe, se ja está logado, etc */
	auth(socket_id, userid);

  /* Cria a thread de sincronização para aquele cliente */
  if(pthread_create( &s_thread, NULL, sync_thread, (void *) userid) != 0){
    printf("[main] ERROR on thread creation.\n");
    exit(1);
  }

	/* Recebe a linha de comando e redireciona para a função objetivo */
	receive_command_client(socket_id, userid);

  /* Mata a thread de sincronização */
  pthread_cancel(s_thread);

	close(socket_id);
	free(new_socket_id);

	return 0;
}

int createSocket(int port){
  int server_socket_id, new_socket_id;
	struct sockaddr_in server_address, client_address;
	socklen_t client_len;
	char buffer[256];

  /* Cria socket TCP para o servidor. */
	if ((server_socket_id = socket(AF_INET, SOCK_STREAM, 0)) == -1){
		printf("[createSocket] ERROR opening socket\n");
    return ERRO;
	}

	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(port);
	server_address.sin_addr.s_addr = INADDR_ANY;
	bzero(&(server_address.sin_zero), 8);

	if (bind(server_socket_id, (struct sockaddr *) &server_address, sizeof(server_address)) < 0){
		printf("[createSyncSocket] ERROR on binding\n");
    return ERRO;
	}

	listen(server_socket_id, 1);


  /* Aceita conexões de clientes */
	client_len = sizeof(struct sockaddr_in);
	if ((new_socket_id = accept(server_socket_id, (struct sockaddr *) &client_address, &client_len)) == -1){
		printf("[createSyncSocket] ERROR on accept\n");
	}

	bzero(buffer, 256);


  return new_socket_id;
}

int main(int argc, char *argv[]){
	int server_socket_id, new_socket_id;
  struct sockaddr_in server_address, client_address;
	socklen_t client_len;

    /* Talvez devêssemos alocar por malloc */
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
	listen(server_socket_id, 5);

	client_len = sizeof(struct sockaddr_in);

	/* Laço que fica aguardando conexões de clientes e criandos as threads*/
	while(1){
	    /* Aguarda a conexão do cliente */
		if((new_socket_id = accept(server_socket_id, (struct sockaddr *) &client_address, &client_len)) != ERRO){
    	/* Aloca dinamicamente para armazenar o número do socket e passar para a thread */
    	int *arg = malloc(sizeof(*arg));
    	*arg = new_socket_id;

    	/* Se conectou, cria a thread para o cliente */
    	if(pthread_create( &c_thread, NULL, client_thread, arg) != 0){
    		printf("[main] ERROR on thread creation.\n");
    		close(new_socket_id);
          exit(1);
      }
    }
    else{
      printf("[main] Erro no accept\n");
      exit(1);
    }
  }

	close(new_socket_id);
	close(server_socket_id);

	clients_list = clearClientsList(clients_list);

	return 0;
}
