#ifndef __DROPBOXUTIL__
#define	__DROPBOXUTIL__

#define MAXNAME 32
#define MAXFILES 256
#define ERRO -1


int writeFileToBuffer(char* filename, char* buffer);
int writeBufferToFile(char* filename, char* buffer);
char* getClientFolderName(char* client_id);
int existsClientFolder(char* client_id);
void sendFileThroughSocket(char *file, char* buffer, int socket);
void receiveFileThroughSocket(char *file, char* buffer, int socket);
int existsFolder(char* path_folder);

#endif
