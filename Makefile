CC = gcc
# Aggiungi -D_DEFAULT_SOURCE se usi be64toh/htobe64 e non sono disponibili altrimenti
# Oppure includi <endian.h> esplicitamente nei .c se necessario.
# Generalmente <arpa/inet.h> o glibc li forniscono.
CFLAGS = -Wall -pthread -Iinclude -g # Aggiunto -g per il debug

# Directory dei sorgenti e degli header (se diverse da quelle di default)
SRC_DIR = src
INCLUDE_DIR = include

# File sorgente per il client
CLIENT_MAIN_SRC = $(SRC_DIR)/main_client.c
CLIENT_UTIL_SRC = $(SRC_DIR)/client.c
CLIENT_SRCS = $(CLIENT_MAIN_SRC) $(CLIENT_UTIL_SRC)

# File sorgente per il server
SERVER_MAIN_SRC = $(SRC_DIR)/main_server.c
SERVER_UTIL_SRC = $(SRC_DIR)/server.c
SERVER_UTILS_SRC = $(SRC_DIR)/server_utils.c
SERVER_SRCS = $(SERVER_MAIN_SRC) $(SERVER_UTIL_SRC) $(SERVER_UTILS_SRC)

# Eseguibili
CLIENT_OUT = client.out
SERVER_OUT = server.out

# Headers (usati per le dipendenze, potrebbero essere più specifici per target)
# Per semplicità, elenchiamo quelli principali. Make può inferire alcune dipendenze.
# HEADERS = $(INCLUDE_DIR)/client.h $(INCLUDE_DIR)/server.h $(INCLUDE_DIR)/server_utils.h $(INCLUDE_DIR)/utils.h

all: $(CLIENT_OUT) $(SERVER_OUT)

# Regola per il client
$(CLIENT_OUT): $(CLIENT_SRCS) $(INCLUDE_DIR)/client.h $(INCLUDE_DIR)/utils.h
	$(CC) $(CFLAGS) -o $(CLIENT_OUT) $(CLIENT_SRCS)

# Regola per il server
$(SERVER_OUT): $(SERVER_SRCS) $(INCLUDE_DIR)/server.h $(INCLUDE_DIR)/server_utils.h $(INCLUDE_DIR)/utils.h
	$(CC) $(CFLAGS) -o $(SERVER_OUT) $(SERVER_SRCS)

clean:
	rm -f $(CLIENT_OUT) $(SERVER_OUT) *.o # Rimuove anche eventuali file oggetto

.PHONY: all clean