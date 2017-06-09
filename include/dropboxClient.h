#ifndef __DROPBOXCLIENT__
#define	__DROPBOXCLIENT__

#define MAXNAME 32
#define MAXFILES 256
#define ERRO -1


int connect_server(char *host, int port);
void close_connection();
void sync_client();
void list(char* line, int socket);
void send_file(char *file, char* buffer, int socket);
void get_file(char *file, char* line, int socket);
int auth(int socket, char* userid);

#endif
