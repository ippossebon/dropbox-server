#ifndef __DROPBOXCLIENT__
#define	__DROPBOXCLIENT__

#define MAXNAME 32
#define MAXFILES 256
#define ERRO -1

int connect_server(char *host, int port);
void close_connection(char* buffer, SSL *ssl);
void sync_client();
void list(char* line, SSL *ssl);
void send_file(char *file, char* buffer, SSL *ssl);
void get_file(char *file, SSL *ssl);
int auth(SSL *ssl, char* userid);
int get_sync_dir();
void *sync_thread(void*);
char* get_timestamp_server(int socket);
file_node* fn_create_from_path_server_time(char* path, int socket);

#endif
