#!/bin/bash

# Avvia il server in background
./server -p 4 -s test_output -l 5 -P 8080 &
SERVER_PID=$!

# Aspetta che il server si avvii
sleep 2

# Invia un segnale di interruzione
kill -SIGINT $SERVER_PID

# Verifica che il server termini correttamente
if ps -p $SERVER_PID > /dev/null; then
    echo "Errore: Server non terminato correttamente"
    exit 1
fi

echo "Test gestione segnali completato"