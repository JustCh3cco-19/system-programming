# System Programming – Secure Client-Server File Transfer

A modular client-server architecture in C for secure file transfer using 64-bit XOR symmetric encryption.  
Data is sent over TCP sockets, with concurrent connections on the server and parallelized encryption/decryption using POSIX threads (pthreads).

---

## Features

- **Secure File Transmission:**  
 Files are encrypted and sent over TCP with a 64-bit symmetric XOR cipher.

- **Concurrent Server:**  
 Handles multiple simultaneous connections using `fork()` or `pthreads`.

- **Parallel Processing:**  
 Both client and server use POSIX threads to split encryption/decryption across blocks.

- **Structured Protocol:**  
 Message contains encrypted data blocks, file size, and the symmetric key.

- **Robust Error Handling:**  
 Gracefully manages file, socket, and protocol errors; clean shutdowns on signals.

- **Modular Design:**  
 Separate modules for client, server, and shared utilities (logging, parsing, synchronization).

---

## Architecture

### System Components

- **Client module:**  
 Reads input file, splits into 64-bit blocks, encrypts in parallel, sends over TCP.

- **Server module:**  
 Receives encrypted data, decrypts in parallel, and saves output to file. Sends ACK on completion.

- **Utility module:**  
 Shared helpers for argument parsing, logging, signal handling, and mutex-protected operations.

### Communication Protocol

The client sends a structured buffer containing:
- **B:** Sequence of 64-bit encrypted blocks (`uint64_t`)
- **L:** Unsigned integer representing the actual file length in bytes
- **K:** 64-bit symmetric key used for encryption

### Encryption Algorithm

Uses bitwise XOR operation: `C_i = P_i ⊕ K`  
where `P_i` is the i-th plaintext block and `K` is the 64-bit key.

### Concurrency and Synchronization

- Parallelism implemented with `pthread_t`, assigning each thread a range of blocks to encrypt/decrypt
- Shared data structures protected with `pthread_mutex_t`
- Asynchronous signals handled via `sigaction` and `sigset_t` to prevent race conditions

---

## Build

To compile all modules (client, server, and utilities):

# Compile separately:
```sh
gcc src/client.c src/utils.c -Iinclude -lpthread -o client.out
gcc src/server.c src/utils.c -Iinclude -lpthread -o server.out
```
---

## Usage

### Server
```sh
./server.out <max_threads> <output_prefix> <max_connections> <port>
```

**Parameters:**
- `max_threads`: Maximum number of threads for parallel decryption
- `output_prefix`: Prefix for output filenames (e.g., "s" or "out")
- `max_connections`: Maximum concurrent client connections
- `port`: TCP port to listen on

### Client
```sh
./client.out <input_file> <encryption_key> <threads> <server_ip> <server_port>
```

**Parameters:**
- `input_file`: Path to file to encrypt and send
- `encryption_key`: 64-bit hexadecimal key (e.g., 0x0123456789ABCDEF)
- `threads`: Number of threads for parallel encryption
- `server_ip`: Server IP address (e.g., 127.0.0.1)
- `server_port`: Server port number

---

## Testing

Navigate to the `test` directory and run the following test cases:

### Test 1: Basic functionality
Server: ./server.out 2 s 2 12345  
Client: ../client.out sample1.txt 0x0123456789ABCDEF 2 127.0.0.1 12345

### Test 2: Different key and file
Server: ./server.out 2 s 2 12345  
Client: ../client.out sample2.txt 0x0F0E0D0C0B0A0908 2 127.0.0.1 12345

### Test 3: Increased parallelism
Server: ./server.out 4 s 2 12345  
Client: ../client.out sample3.txt 0xA1B2C3D4E5F60708 4 127.0.0.1 12345

### Test 4: Binary file transfer
Server: ./server.out 4 s 2 12345  
Client: ../client.out random_1k.bin 0xCAFEBABEDEADBEEF 4 127.0.0.1 12345

### Test 5: Zero key test
Server: ```sh ./server.out 2 s 2 12345 ```
  
Client: ```sh ../client.out sample4.txt 0x0000000000000000 2 127.0.0.1 12345 ```

### Test 6: Multiple concurrent connections
Server: ```sh ./server.out 4 s 5 12345 ```  
Client (run in different terminals):
```sh
../client.out C1_1.bin 0x1111111111111111 4 127.0.0.1 12345 &
../client.out C1_2.bin 0x1111111111111112 4 127.0.0.1 12345 &
../client.out C1_3.bin 0x1111111111111113 4 127.0.0.1 12345 &
../client.out C1_4.bin 0x1111111111111114 4 127.0.0.1 12345 &
../client.out C1_5.bin 0x1111111111111115 4 127.0.0.1 12345 &
```

### Test 7: Different output prefix
Server: ```sh ./server.out 4 out 2 12345 ```  
Client:
```sh
../client.out C2_1.bin 0x2222222222222221 4 127.0.0.1 12345 &
../client.out C2_2.bin 0x2222222222222222 4 127.0.0.1 12345 &
../client.out C2_3.bin 0x2222222222222223 4 127.0.0.1 12345 &
```

### Test 8: Large file with high parallelism
Server: 
```sh
./server.out 8 out 2 12345  
```
Client: 
```sh
../client.out big.bin 0xDEADBEEFCAFEBABE 8 127.0.0.1 12345 &  
CLIENT_PID=$!
```

### Test 9: Background server process
Server: ```sh ./server.out 4 s 2 12345 & ```
  
SERVER_PID=$!  
Client: ```sh ../client.out mid.bin 0xFEEDFACECAFEBABE 4 127.0.0.1 12345 ```

### Test 10: Edge case with large key
Server: ```sh ./server.out 4 s 2 12345 ```  
Client: ```sh ../client.out ok.txt 0x1FFFFFFFFFFFFFFFF 2 127.0.0.1 12345 ```

### Server Management
To stop the server:
```sh
ps aux | grep server.out
kill -9 <pid>
```

---

## Validation

The system has been tested for:
- **Data Integrity:** Verification using `diff` between original and decrypted files
- **Server Stability:** Performance under concurrent connection load
- **Thread Distribution:** Proper work allocation among threads
- **Error Handling:** Graceful handling of various error conditions

---

## Implementation Details

### Client Pipeline
1. Parse command-line arguments (file, key, parallelism, server IP/port)
2. Allocate memory and read file content
3. Split into 64-bit blocks and perform concurrent encryption with `pthread_create`
4. Compose message and send via `send()` on TCP socket
5. Close socket and deallocate memory

### Server Pipeline
1. Start listening on defined port with maximum concurrent connections via `fork()` or `pthread`
2. Receive message: sequential reading of encrypted blocks, size, and key
3. Concurrent decryption with `pthread` (same algorithms as client but in reverse)
4. Write content to file (dynamically generated filename based on prefix and unique ID)
5. Send confirmation (ACK) and close connection
