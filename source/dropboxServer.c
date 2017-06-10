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

char username[MAXNAME];
client_node* clients_list;

/* Sincroniza o servidor com o diretório “sync_dir_<nomeusuário>”
com o cliente. */
void sync_server(){
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
void get_sync_dir(char* userid){
	printf("Chegou no get_sync_dir com client_id: %s\n", userid);

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
    }
    char buffer[256];
    strcat(buffer, "exists");

    int num_bytes_sent;
    int size = strlen(buffer);

    /* Envia para o client os nomes dos arquivos */
    num_bytes_sent = write(sync_socket, buffer, size);
    if (num_bytes_sent < 0){
        printf("[get_sync_dir] Erro ao escrever no socket\n");
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
        printf("[get_sync_dir] Não existe pasta sync_dir_user no device.\n");

        /* Deve criar a pasta no diretório do cliente. */

    }
    else{
        printf("[get_sync_dir] Existe pasta sync_dir_user no device.\n");
    }
    exit(0);
}

/* Recebe um arquivo file do cliente.
Deverá ser executada quando for realizar upload de um arquivo.
file – path/filename.ext do arquivo a ser recebido */
void receive_file(char *file_name, char* file_data){
  /* file_data contém o conteúdo do arquivo a ser enviado para o servidor, e
	filename é o nome do arquivo que está sendo enviado. */
	char *full_path = getClientFolderName(username);
	strcat(full_path, file_name);

    //printf("Folder do cliente: %s\n", full_path);
	/* Cria um novo arquivo, na pasta do cliente logado, com o nome do arquivo
	informado, com o conteúdo do arquivo enviado pelo socket. */
	writeBufferToFile(full_path, file_data);
}

/* Envia o arquivo file para o usuário.
Deverá ser executada quando for realizar download de um arquivo.
file – filename.ext */
void send_file(char *file_name, int socket){
	char* full_path = getClientFolderName(username);
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
void list(int socket){

    char* full_path = getClientFolderName(username);
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


void auth(int socket){
    char buffer[256];
    bzero(buffer, 256);
    int num_bytes_read, num_bytes_sent;

    /* No buffer vem o userid do cliente que esta tentando conectar */
    num_bytes_read = read(socket, buffer, 256);
    if (num_bytes_read < 0){
        printf("[auth] ERROR reading from socket \n");
    }

    strcpy(username, buffer);

	/* Verifica se o usuário em questão já é cadastrado ou se é um novo usuário.
	Se for um novo usuário, cria uma struct para ele e adiciona-o a lista de clientes.
	Caso contrário, verifica se o número de dispositivos logados é respeitado.
	Se tudo estiver ok, envia mensagem de sucesso para o cliente e prossegue com
	as operações. */
	client* user = findUserInClientsList(clients_list, username);

	if (user == NULL){
		/* O usuário ainda não é cadastrado. */
		struct client new_client;
		strcpy(new_client.userid, username);

		/*TODO: como inicializar a variavel devices? */
		new_client.devices[0] = 0;
		new_client.devices[1] = 0;

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
    printf("Saindo da funcao de autentificacao\n");
}

void receive_command_client(int socket){
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
            receive_file(file_name, file_data);
        }
        else if( strcmp("download", command) == 0){
            send_file(file_name, socket);
        }
        else if( strcmp("list", command) == 0){
            list(socket);
        }
        else if( strcmp("get_sync_dir", command) == 0){
            //sync_client()
        }
        else if( strcmp("exit", command) == 0){
            return;
        }
    }
}

void *client_thread(void *new_socket_id){
	/* Uns casts muito loucos */
	int socket_id = *((int *) new_socket_id);

	/* Faz verificação de usuário. TODO: verificar se existe, se ja está logado, etc */
	auth(socket_id);

    /* Após cada login do usuário, get_sync_dir deve ser chamado. */
    //get_sync_dir(username);


	/* Recebe a linha de comando e redireciona para a função objetivo */
	receive_command_client(socket_id); 
          
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

    /* Informa que o socket em questões pode receber conexões, 5 indica o
    tamanho da fila de mensagens */
	listen(server_socket_id, 5);

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
