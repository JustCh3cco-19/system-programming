import socket
import struct

def test_invalid_key_length():
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.connect(('127.0.0.1', 8080))
        # Invia una chiave di lunghezza errata
        s.sendall(b'SHORT')
        ack = s.recv(1)
        assert ack[0] == 0, "Server ha accettato una chiave invalida"
    print("Test chiave breve completato")

def test_disconnect_mid_transfer():
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.connect(('127.0.0.1', 8080))
        s.sendall(b'KEY12345')  # Chiave valida
        # Chiudi la connessione prima di inviare il resto
    print("Test disconnessione a metà completato")

def test_large_file():
    # Simula un file molto grande
    key = b'KEY12345'
    plaintext = b'A' * 1024 * 1024 * 10  # 10 MB
    block_count = (len(plaintext) + 7) // 8
    
    ciphertext = bytearray(len(plaintext))
    for i in range(len(plaintext)):
        ciphertext[i] = plaintext[i] ^ key[i % len(key)]
    
    padding_len = block_count * 8 - len(plaintext)
    if padding_len > 0:
        ciphertext += bytes([padding_len] * padding_len)
    
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.connect(('127.0.0.1', 8080))
        s.sendall(key)
        s.sendall(struct.pack('!I', block_count))
        s.sendall(ciphertext)
        ack = s.recv(1)
        assert ack[0] == 1, "NACK per file grande"
    print("Test file grande completato")

if __name__ == '__main__':
    test_invalid_key_length()
    test_disconnect_mid_transfer()
    test_large_file()