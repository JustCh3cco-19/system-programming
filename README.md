# system-programming

## Compilare
make

### Utilizzo codice
server: ./server.out 4 out 2 12345

client: ./client.out sample.txt 0xA5A5A5A5A5A5A5A5 4 127.0.0.1 12345

## Per killare il server 
ps aux | grep server.out

kill -9 <id_pid>