#!/bin/bash

# =============================
# Script per compilare il server e i test
# Uso: chmod +x build.sh && ./build.sh
# =============================

# Colori per output leggibile
GREEN='\033[0;32m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}Inizio compilazione...${NC}"

# Creiamo la directory per gli oggetti se non esiste
mkdir -p src

# Compila le funzioni comuni (server_utils.o)
echo -e "${GREEN}[1/3] Compilando server_utils.c...${NC}"
gcc -c src/server_utils.c -o src/server_utils.o \
    -Wall -Wextra -g -Iinclude -lpthread -lrt

if [ $? -ne 0 ]; then
    echo -e "\n❌ Errore nella compilazione di server_utils.c"
    exit 1
fi

# Compila il server
echo -e "${GREEN}[2/3] Compilando il server...${NC}"
gcc src/server.c src/server_utils.o -o server.out \
    -Wall -Wextra -g -Iinclude -lpthread -lrt

if [ $? -ne 0 ]; then
    echo -e "\n❌ Errore nella compilazione del server"
    exit 1
fi

# Compila i test unitari
echo -e "${GREEN}[3/3] Compilando i test unitari...${NC}"
gcc src/test_decrypt_blocks.c src/server_utils.o -o test_decrypt_blocks.out \
    -Wall -Wextra -g -Iinclude -lpthread -lrt

if [ $? -ne 0 ]; then
    echo -e "\n❌ Errore nella compilazione dei test"
    exit 1
fi

echo -e "\n✅ Compilazione completata con successo!"
echo -e "Binari generati:"
echo -e "- Server: ./server.out"
echo -e "- Test: ./test_decrypt_blocks.out"