import socket
import struct
import os

def run_test():
    HOST = '127.0.0.1'
    PORT = 8080
    
    # Dati di test
    key = b'KEY12345'
    plaintext = b'Test message with padding\x04\x04\x04\x04'
    block_count = (len(plaintext) + 7) // 8  # Arrotonda al blocco più vicino
    
    # Crea un ciphertext usando XOR con la chiave
    ciphertext = bytearray(len(plaintext))
    for i in range(len(plaintext)):
        ciphertext[i] = plaintext[i] ^ key[i % len(key)]
    
    # Riempi fino alla dimensione del blocco
    padding_len = block_count * 8 - len(plaintext)
    if padding_len > 0:
        ciphertext += bytes([padding_len] * padding_len)
    
    # Connessione al server
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.connect((HOST, PORT))
        
        # Invia la chiave
        s.sendall(key)
        
        # Invia il numero di blocchi
        s.sendall(struct.pack('!I', block_count))
        
        # Invia i dati cifrati
        s.sendall(ciphertext)
        
        # Ricevi ACK
        ack = s.recv(1)
        assert ack[0] == 1, "NACK ricevuto dal server"
    
    # Verifica che il file esista
    output_files = [f for f in os.listdir('.') if f.startswith('test_output_')]
    assert len(output_files) >= 1, "Nessun file di output generato"
    
    # Leggi e verifica il contenuto
    with open(output_files[0], 'rb') as f:
        recovered = f.read()
    assert recovered == plaintext, "Contenuto non corrispondente"
    
    print("Test client-server completato con successo")

if __name__ == '__main__':
    run_test()