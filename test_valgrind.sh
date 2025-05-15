#!/bin/bash

valgrind --leak-check=full \
         --show-leak-kinds=all \
         --track-origins=yes \
         --verbose \
         ./server -p 2 -s test -l 3 -P 8080 &
SERVER_PID=$!

sleep 2
# Esegui un test client
python3 test_client.py

kill $SERVER_PID
wait $SERVER_PID 2>/dev/null

echo "Test Valgrind completato"