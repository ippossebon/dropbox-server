CC=gcc
CLIENT_DIR=./client/
INC_DIR=./include/
SERVER_DIR=./server/
FLAGS= -g -Wall

all: utilities server client

utilities: dropboxUtil.o
	$(CC) dropboxUtil.c -c $(FLAGS)

client: $(CLIENT_DIR)dropboxClient.o utilities
	$(CC) -o dropboxClient $(CLIENT_DIR)dropboxClient.c dropboxUtil.o

server: $(SERVER_DIR)dropboxServer.c utilities
	$(CC) -I$(INC_DIR) -o dropboxServer $(SERVER_DIR)dropboxServer.c $(FLAGS)

clean:
	rm -rf $(LIB_DIR)/*.a *.o $(SRC_DIR)/*~ $(INC_DIR)/*~ *~
