CC = gcc
CFLAGS = -Wall -pthread -Iinclude

all: client server

# Regola per il client
client: src/client.c include/client.h include/utils.h
	$(CC) $(CFLAGS) -o client.out src/client.c

# Regola per il server
server: src/server.c src/server_utils.c include/server.h include/server_utils.h include/utils.h
	$(CC) $(CFLAGS) -o server.out src/server.c src/server_utils.c

clean:
	rm -f client.out server.out