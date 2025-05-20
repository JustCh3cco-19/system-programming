CC    = gcc
CFLAGS= -Wall -Wextra -Iinclude -pthread -std=c11
SRC   = src/client.c src/server.c src/utils.c
OBJ   = $(SRC:.c=.o)

all: client server

client: src/client.o src/utils.o
	$(CC) $(CFLAGS) -o client.out src/client.o src/utils.o

server: src/server.o src/utils.o
	$(CC) $(CFLAGS) -o server.out src/server.o src/utils.o

src/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f src/*.o client server
