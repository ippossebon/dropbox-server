#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

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

    return full_path;
}

int existsClientFolder(char* client_id){
    char* path = getClientFolderName(client_id);

    if (stat(path, &st) == -1) {
        return 0;
    }
    else{
        return 1;
    }
}

void sendFileThroughSocket(char *file, char* buffer, int socket){

    int aux;

    aux = writeFileToBuffer(file, buffer);
    if (aux != 0){
        printf("Erro ao abrir arquivo.\n");
    }

    printf("[sendFileThroughSocket] buffer para enviar: %s\n", buffer);
    /* Envia conteúdo do buffer pelo socket */
    int num_bytes_sent;
    int buffer_size = strlen(buffer);
	  num_bytes_sent = write(socket, buffer, buffer_size);

    if (num_bytes_sent < 0){
        printf("ERROR writing to socket\n");
    }

    printf("[sendFileThroughSocket] num_bytes_sent: %d\n", num_bytes_sent);

    bzero(buffer,256);
}

void receiveFileThroughSocket(char* file, char* buffer, int socket){

    int num_bytes_read;
	bzero(buffer, 256);
    num_bytes_read = read(socket, buffer, 256);

    if (num_bytes_read < 0){
        printf("ERROR reading from socket");
    }

    writeBufferToFile(file, buffer);
}
