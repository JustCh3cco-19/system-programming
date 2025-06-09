# system-programming

## Compilare
client: gcc -Iinclude -pthread src/client.c src/utils.c -o client.out
server: gcc -Iinclude -pthread src/server.c src/utils.c -o server.out


### Testing
cd test

Da terminale avviare i seguenti comandi
Test1:
server: ./server.out 2 s 2 12345
client: ../client.out sample1.txt 0x0123456789ABCDEF 2 127.0.0.1 12345

Test2:
server: ./server.out 2 s 2 12345
client: ../client.out sample2.txt 0x0F0E0D0C0B0A0908 2 127.0.0.1 12345

Test3:
server: ./server.out 4 s 2 12345
client: ../client.out sample3.txt 0xA1B2C3D4E5F60708 4 127.0.0.1 12345

Test4:
server: ./server.out 4 s 2 12345
client: ../client.out random_1k.bin 0xCAFEBABEDEADBEEF 4 127.0.0.1 12345

Test5:
server: ./server.out 2 s 2 12345
client: ../client.out sample4.txt 0x0000000000000000 2 127.0.0.1 12345

Test6:
server: ./server.out 4 s 5 12345
In terminali diversi avviare tutti questi test:
../client.out C1_1.bin 0x1111111111111111 4 127.0.0.1 12345 &
../client.out C1_2.bin 0x1111111111111112 4 127.0.0.1 12345 &
../client.out C1_3.bin 0x1111111111111113 4 127.0.0.1 12345 &
../client.out C1_4.bin 0x1111111111111114 4 127.0.0.1 12345 &
../client.out C1_5.bin 0x1111111111111115 4 127.0.0.1 12345 &

Test7:
server: ./server.out 4 out 2 12345
client:
../client.out C2_1.bin 0x2222222222222221 4 127.0.0.1 12345 &
../client.out C2_2.bin 0x2222222222222222 4 127.0.0.1 12345 &
../client.out C2_3.bin 0x2222222222222223 4 127.0.0.1 12345 &

Test8:
server: ./server.out 8 out 2 12345
client: ../client.out big.bin 0xDEADBEEFCAFEBABE 8 127.0.0.1 12345 & CLIENT_PID=$!

Test9:
server: ./server.out 4 s 2 12345 & SERVER_PID=$!
client: ../client.out mid.bin 0xFEEDFACECAFEBABE 4 127.0.0.1 12345

Test10:
server: ./server.out 4 s 2 12345
client: ../client.out ok.txt 0x1FFFFFFFFFFFFFFFF 2 127.0.0.1 12345
## Per killare il server 
ps aux | grep server.out

kill -9 <server_id_pid>