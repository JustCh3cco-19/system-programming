#!/bin/bash

# Parametri invalidi
./server -p -1 -s test -l 5 -P 8080 2>&1 | grep -q "Invalid parameters"
echo "Test parametri invalidi completato"

# Porta non disponibile
nc -z 127.0.0.1 8080 && echo "Server già in esecuzione" || echo "Porta libera"

# Massimo numero di connessioni
for i in {1..10}; do
    nc -z 127.0.0.1 8080 > /dev/null 2>&1 && echo "Connessione $i riuscita" || echo "Connessione $i fallita"
done

echo "Test casi limite completati"