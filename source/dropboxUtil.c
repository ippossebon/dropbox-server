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
int existsClientFolder(char* path){
    if (stat(path, &st) == -1) {
        return 0;
    }
    else{
        return 1;
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
