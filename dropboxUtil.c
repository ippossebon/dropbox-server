#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Lê um arquivo local e escreve no buffer. */
int writeFileToBuffer(char* filename, char* buffer){
    FILE *file;
    file = fopen(filename, "r");
    int i = 0;

    if (file == NULL){
        return 1;
    }
    else{
        int c;
        while ((c = getc(file)) != EOF){
            buffer[i] = c;
            i++;
        }
    fclose(file);
    }

    return 0;
}
