#ifndef __DROPBOXUTIL__
#define	__DROPBOXUTIL__

#define MAXNAME 32
#define MAXFILES 256
#define MAXCLIENTS 5
#define ERRO -1
#define SUCESSO 1
#define BUF_SIZE 256

//Estrutura que vai armazenar informações de um arquivo
typedef struct file_info {
	char name[MAXNAME]; // name[MAXNAME] refere-se ao nome do arquivo
	char extension[MAXNAME]; // extension[MAXNAME] refere-se ao tipo de extensão do arquivo.
	char last_modified[MAXNAME]; // last_modified [MAXNAME] refere-se a data da última mofidicação no arquivo
	int size; // size indica o tamanho do arquivo, em bytes.
} file_info;

//Estrutura de nó para armezenar uma lista encadeada de arquivos
typedef struct file_node {
   file_info* data;
   struct file_node* next;
} file_node;

/* Estrutura para lista de clientes cadastrados */
typedef struct client {
    int devices[2]; // associado aos dispositivos do usuário
	char userid[MAXNAME]; //  id do usuário no servidor, que deverá ser único. Informado pela linha de comando.
	file_node* files;
    int logged_in; // cliente está logado ou não.
} client;

typedef struct client_node{
    client* client;
    struct client_node* next;
} client_node;

/* Estrutura para armazenar os sockets a serem passados pra thread */
typedef struct arg_struct {
    int socket_id;
    int sync_socket;
} arg_struct;

/* Estrutura para armazenar os sockets a serem passados pra thread de sync*/
typedef struct arg_struct_sync {
    int sync_socket;
    char userid[MAXNAME];
} arg_struct_sync;

int writeFileToBuffer(char* filename, char* buffer);
int writeBufferToFile(char* filename, char* buffer);
char* getClientFolderName(char* client_id);
int existsFolder(char* path);

file_node* fn_create(); //Cria uma lista de arquivos vazia
file_node* fn_create_from_path(char* path); //Cria uma lista a partir dos arquivos de um caminho indicado por path
file_node* fn_add(file_node* list, file_info* file); //Adiciona um arquivo a lista
file_node* fn_del(file_node* list, char* filename); //Remove o arquivo com o nome filename da lista.
file_info* fn_find(file_node* list, char *filename); //Busca um arquivo pelo nome (filename). Se não encontrar retorna NULL.
file_node* fn_clear(file_node* list); //Limpa toda a lista
void fn_print(file_node* list); //Imprime toda a lista para fins de debug

client_node* createClientsList();
client_node* addClientToList(client_node* list, client* user);
client* findUserInClientsList(client_node* list, char *username);
client_node* clearClientsList(client_node* list);
client_node* removeClientFromList(client_node* list, char* username);
void printClientsList(client_node* list);

#endif
