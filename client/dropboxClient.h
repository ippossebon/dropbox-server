#ifndef __DROPBOXCLIENT__
#define	__DROPBOXCLIENT__


int connect_server(char *host, int port);
void sync_client();
void send_file(char *file);
void get_file(char *file);
void close_connection();

#endif
