#!/bin/bash

SERVER="./server/server"
CLIENT="./client/client"

# Configurazione server
PORT=12345
IP="127.0.0.1"
P_SERVER=4
L_CONNECTIONS=5
PREFIX="output_"

# File di test
TEST_DIR="test_files"
mkdir -p "$TEST_DIR"

# Creiamo alcuni file di test
echo "Questo è un testo semplice." > "$TEST_DIR/test1.txt"
dd if=/dev/zero bs=1 count=10 > "$TEST_DIR/test2.bin" 2>/dev/null
head /dev/urandom -c 15 > "$TEST_DIR/test3_partial.bin"
cat /etc/passwd > "$TEST_DIR/test4_larger.txt"

KEY="0x123456789ABCDEF"

# Funzione per attendere che il server sia pronto
wait_for_server() {
    local max_attempts=10
    local attempt=0
    while [ $attempt -lt $max_attempts ]; do
        nc -zv "$IP" "$PORT" &>/dev/null && return 0
        sleep 0.5
        ((attempt++))
    done
    echo "[ERROR] Server non raggiungibile sulla porta $PORT"
    exit 1
}

# Funzione per generare nome file atteso
get_expected_output() {
    find . -name "${PREFIX}*.tmp" | sort | tail -n1
}

# Test 1: Invio di un file di testo normale
test1() {
    echo "Test 1: File di testo normale"
    cp "$TEST_DIR/test1.txt" input.tmp
    "$SERVER" "$P_SERVER" "$PREFIX" "$L_CONNECTIONS" &
    SERVER_PID=$!
    wait_for_server
    "$CLIENT" input.tmp "$KEY" 4 "$IP" "$PORT"
    kill $SERVER_PID
    wait $SERVER_PID 2>/dev/null

    FILE_OUT=$(get_expected_output)
    if cmp input.tmp "$FILE_OUT"; then
        echo "Test 1 passato"
        rm -f input.tmp "$FILE_OUT"
        return 0
    else
        echo "Test 1 fallito"
        rm -f input.tmp "$FILE_OUT"
        return 1
    fi
}

# Test 2: Invio di file binario (byte zero)
test2() {
    echo "Test 2: File binario (zeri)"
    cp "$TEST_DIR/test2.bin" input.tmp
    "$SERVER" "$P_SERVER" "$PREFIX" "$L_CONNECTIONS" &
    SERVER_PID=$!
    wait_for_server
    "$CLIENT" input.tmp "$KEY" 4 "$IP" "$PORT"
    kill $SERVER_PID
    wait $SERVER_PID 2>/dev/null

    FILE_OUT=$(get_expected_output)
    if cmp input.tmp "$FILE_OUT"; then
        echo "Test 2 passato"
        rm -f input.tmp "$FILE_OUT"
        return 0
    else
        echo "Test 2 fallito"
        rm -f input.tmp "$FILE_OUT"
        return 1
    fi
}

# Test 3: File con lunghezza non multipla di 8 byte
test3() {
    echo "Test 3: File con padding (non multiplo di 8 byte)"
    cp "$TEST_DIR/test3_partial.bin" input.tmp
    "$SERVER" "$P_SERVER" "$PREFIX" "$L_CONNECTIONS" &
    SERVER_PID=$!
    wait_for_server
    "$CLIENT" input.tmp "$KEY" 4 "$IP" "$PORT"
    kill $SERVER_PID
    wait $SERVER_PID 2>/dev/null

    FILE_OUT=$(get_expected_output)

    # Rimuovi padding prima del confronto
    ORIGINAL_LEN=$(wc -c < input.tmp)
    dd if="$FILE_OUT" of="$FILE_OUT.trunc" bs=1 count=$ORIGINAL_LEN 2>/dev/null

    if cmp input.tmp "$FILE_OUT.trunc"; then
        echo "Test 3 passato"
        rm -f input.tmp "$FILE_OUT"* "$FILE_OUT.trunc"
        return 0
    else
        echo "Test 3 fallito"
        rm -f input.tmp "$FILE_OUT"* "$FILE_OUT.trunc"
        return 1
    fi
}

# Test 4: File grande (> 1 blocco)
test4() {
    echo "Test 4: File grande"
    cp "$TEST_DIR/test4_larger.txt" input.tmp
    "$SERVER" "$P_SERVER" "$PREFIX" "$L_CONNECTIONS" &
    SERVER_PID=$!
    wait_for_server
    "$CLIENT" input.tmp "$KEY" 4 "$IP" "$PORT"
    kill $SERVER_PID
    wait $SERVER_PID 2>/dev/null

    FILE_OUT=$(get_expected_output)
    if cmp input.tmp "$FILE_OUT"; then
        echo "Test 4 passato"
        rm -f input.tmp "$FILE_OUT"
        return 0
    else
        echo "Test 4 fallito"
        rm -f input.tmp "$FILE_OUT"
        return 1
    fi
}

# Test 5: Server offline
test5() {
    echo "Test 5: Tentativo di connessione a server offline"
    "$CLIENT" "$TEST_DIR/test1.txt" "$KEY" 4 "$IP" "$PORT" 2>&1 | grep -q "Connection refused"
    if [ $? -eq 0 ]; then
        echo "Test 5 passato"
        return 0
    else
        echo "Test 5 fallito"
        return 1
    fi
}

# Lancio dei test
echo "Inizio test funzionali"
echo "-------------------------------"

passed=0
total=0

for test in test1 test2 test3 test4 test5; do
    $test
    if [ $? -eq 0 ]; then
        passed=$((passed + 1))
    fi
    total=$((total + 1))
done

echo "-------------------------------"
echo "Test completati: $passed/$total"

if [ $passed -eq $total ]; then
    echo "Tutti i test passati!"
    exit 0
else
    echo "Alcuni test falliti."
    exit 1
fi