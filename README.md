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

- **Client module:**  
  Reads input file, splits into 64-bit blocks, encrypts in parallel, sends over TCP.

- **Server module:**  
  Receives, decrypts in parallel, and saves output to file. Sends ACK on completion.

- **Utility module:**  
  Shared helpers for argument parsing, logging, signal handling, and mutex-protected operations.

---

## Build

To compile all modules (client, server, and utilities):

```sh
gcc src/client.c src/server.c src/utils.c -Iinclude -lpthread -o client_server_app.out
