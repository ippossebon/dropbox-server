#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <dirent.h>
#include <time.h>

#include "../include/dropboxUtil.h"

struct stat st = {0};

/* Lê um arquivo local e escreve no buffer. */
int writeFileToBuffer(char* filename, char* buffer){
    FILE *file;
    file = fopen(filename, "r");
    int i = 0;
    int bufferSize = strlen(buffer);

    if (file == NULL){
        printf("[writeFileToBuffer] Erro ao escrever arquivo no buffer.\n");
        return ERRO;
    }
    else{
        int c;
        while ((c = getc(file)) != EOF){
            buffer[i+bufferSize] = c;
            i++;
        }
    }
    fclose(file);
    return 0;
}

int writeBufferToFile(char* filename, char* buffer){
    FILE *file;
    file = fopen(filename, "w");
    int buffer_size = strlen(buffer);

    if (fwrite(buffer, 1, buffer_size, file) != buffer_size){
        printf("[writeBufferToFile] Erro ao escrever buffer em arquivo.\n");
        return ERRO;
    }

    fclose(file);
    return 0;
}

char* getClientFolderName(char* client_id){
    char* path = "../server/client_folders/";
    char* full_path = malloc(sizeof(path) + (sizeof(char) * MAXNAME));

    strcat(full_path, path);
    strcat(full_path, client_id);
    strcat(full_path, "/");

    return full_path;
}

/* Retorna 0 se não existe o diretório em questão */
int existsFolder(char* path){
    DIR* dir = opendir(path);

    if (dir)
    { // Diretório existe.
        closedir(dir);
        return 1;
    }
    else{
        return 0;
    }
}

file_node* fn_create_from_path(char* path) { //Cria um file_set a partir dos arquivos de um caminho indicado por path

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
               //TODO Tem que considerar milisegundos aqui?
               //Pega a última modificação do arquivo e salva. Ex.: "2017.03.12 08:10:59"
               strftime(file->last_modified, 36, "%Y.%m.%d %H:%M:%S", localtime(&attr.st_mtime));
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

file_node* fn_create() {
   return NULL;
}

file_node* fn_add(file_node* list, file_info* file) {
   file_node* head = malloc(sizeof(file_node));
   head->data = file;
   head->next = list;
   return head;
}

file_info* fn_find(file_node* list, char *filename) {
  file_node* node;

   for (node = list; node !=NULL; node = node->next) {
       file_info* file = node->data;
       if (strcmp(file->name,filename) == 0) {
          return file;
       }
   }
   return NULL;
}

file_node* fn_clear(file_node* list) {
   file_node* node = list;
   while (node!=NULL) {
      file_node* next = node->next;
      //Libera o nó
      free(node->data);
      free(node);
      node = next;
   }
   return NULL;
}

file_node* fn_del(file_node* list, char* filename) {

   file_node* previous = NULL; //Anterior
   file_node* node = list;

   //Procura o arquivo
   while (node!=NULL && strcmp(node->data->name,filename)!=0) {
       previous = node;
       node = node->next;
   }

   if (node == NULL) { //Não achou, retorna a lista original para não dar merda.
      return list;
   }

   if (previous == NULL) { //Retira o primeiro elemento
      list = node->next;
   } else { //Retira do meio
      previous->next = node->next;
   }

   //Desaloca nó removido
   free(node->data);
   free(node);

   return list;
}

void fn_print(file_node* list) {
   printf("----------------------------------\n");
   file_node* node;

   for (node = list; node !=NULL; node = node->next) {
       file_info* file = node->data;
       printf("Filename: %s, Modified: %s, Size: %d bytes\n", file->name, file->last_modified, file->size);
   }
   printf("----------------------------------\n");
}

/***** Funções para tratamento da lista de clientes. *******/
client_node* createClientsList(){
    return NULL;
}

/* Adiciona um novo cliente ao início da lista. */
client_node* addClientToList(client_node* list, client* user) {
   client_node* head = malloc(sizeof(client_node));
   head->client = user;
   head->next = list;

   return head;
}

/* Procura pelo nome de um usuário na lista de clientes cadastrados. */
client* findUserInClientsList(client_node* list, char *username) {
  client_node* node;

   for (node = list; node !=NULL; node = node->next) {
       client* user = node->client;
       if (strcmp(user->userid, username) == 0) {
          return user;
       }
   }
   return NULL;
}

client_node* clearClientsList(client_node* list) {
   client_node* node = list;

   while (node != NULL) {
      client_node* next = node->next;
      //Libera o nó
      free(node->client);
      free(node);
      node = next;
   }

   return NULL;
}

client_node* removeClientFromList(client_node* list, char* username) {

   client_node* previous = NULL;
   client_node* node = list;

   // Procura pelo usuário em questão
   while (node != NULL && strcmp(node->client->userid, username) != 0) {
       previous = node;
       node = node->next;
   }

   if (node == NULL) { // Não achou, então retorna a lista original.
      return list;
   }

   if (previous == NULL) { // Retira o primeiro elemento
      list = node->next;
   } else { // Retira do meio
      previous->next = node->next;
   }

   //Desaloca nó removido
   free(node->client);
   free(node);

   return list;
}

void printClientsList(client_node* list) {
   printf("----------------------------------\n");
   client_node* node;

   for (node = list; node != NULL; node = node->next) {
       client* user = node->client;
       printf("Client name: %s, logged in: %d, device 0: %d, device 1: %d \n",
            user->userid, user->logged_in, user->devices[0], user->devices[1]);
   }
   printf("----------------------------------\n");
}
