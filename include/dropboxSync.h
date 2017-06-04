#ifndef __DROPBOXSYNC__
#define	__DROPBOXSYNC__

#define MAXNAME 32

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


file_node* fn_create(); //Cria uma lista de arquivos vazia 
file_node* fn_create_from_path(char* path); //Cria uma lista a partir dos arquivos de um caminho indicado por path
file_node* fn_add(file_node* list, file_info* file); //Adiciona um arquivo a lista 
file_node* fn_del(file_node* list, char* filename); //Remove o arquivo com o nome filename da lista.
file_info* fn_find(file_node* list, char *filename); //Busca um arquivo pelo nome (filename). Se não encontrar retorna NULL.
file_node* fn_clear(file_node* list); //Limpa toda a lista
void fn_print(file_node* list); //Imprime toda a lista para fins de debug

#endif //__DROPBOXSYNC__
