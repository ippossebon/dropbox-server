#ifndef __DROPBOXCLIENT__
#define	__DROPBOXCLIENT__

#define MAXNAME 32
#define MAXFILES 256
#define ERRO -1


int connect_server(char *host, int port);
void sync_client();
void send_file(char *file, char *tag, int socket);
void get_file(char *file);
void close_connection();

#endif
