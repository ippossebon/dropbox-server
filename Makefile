CC=gcc
CLIENT_DIR=./client/
INC_DIR=./include/
BIN_DIR=./bin/
SERVER_DIR=./server/
FLAGS= -g -Wall

all: utilities server client

utilities: dropboxUtil.c
	$(CC) -I$(INC_DIR) -o $(BIN_DIR)dropboxUtil.o -c dropboxUtil.c $(FLAGS)

server: $(SERVER_DIR)dropboxServer.c utilities
	$(CC) -I$(INC_DIR) -o $(BIN_DIR)dropboxServer.o -c $(SERVER_DIR)dropboxServer.c $(FLAGS)

client: $(CLIENT_DIR)dropboxClient.c utilities
	$(CC) -I$(INC_DIR) -o $(BIN_DIR)dropboxClient.o -c $(CLIENT_DIR)dropboxClient.c $(FLAGS)

clean:
	rm -rf $(LIB_DIR)/*.a $(BIN_DIR)/*.o $(SRC_DIR)/*~ $(INC_DIR)/*~ *~
