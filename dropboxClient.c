#include <stdio.h>
#include <stdlib.h>

/* Conecta o cliente com o servidor.
host – endereço do servidor
port – porta aguardando conexão */
int connect_server(char *host, int port){
    return 0;
}

/* Sincroniza o diretório “sync_dir_<nomeusuário>” com
o servidor */
void sync_client(){

}

/* Envia um arquivo file para o servidor. Deverá ser
executada quando for realizar upload de um arquivo.
file – path/filename.ext do arquivo a ser enviado */
void send_file(char *file){

}


/* Obtém um arquivo file do servidor.
Deverá ser executada quando for realizar download
de um arquivo.
file –filename.ext */
void get_file(char *file){

}

/* Fecha a conexão com o servidor */
void close_connection(){

}
