#include "server.h"
#include <assert.h>

void test_decrypt_blocks() {
    uint8_t ciphertext[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07};
    uint8_t key[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    uint8_t plaintext[8];
    
    decrypt_blocks(ciphertext, key, plaintext, 0, 1);
    
    for (int i = 0; i < 8; i++) {
        assert(plaintext[i] == (~ciphertext[i] & 0xFF));
    }
    printf("decrypt_blocks test passed\n");
}

void test_remove_padding() {
    // Testo con padding valido
    uint8_t data1[] = {0x01, 0x02, 0x03, 0x03, 0x03};
    assert(remove_padding(data1, 5) == 2);
    
    // Testo senza padding
    uint8_t data2[] = {0x01, 0x02, 0x03, 0x04, 0x05};
    assert(remove_padding(data2, 5) == 5);
    
    // Padding non uniforme
    uint8_t data3[] = {0x01, 0x02, 0x03, 0x04, 0x05};
    assert(remove_padding(data3, 5) == 5);
    
    // Padding troppo lungo
    uint8_t data4[] = {0x01, 0x09, 0x09, 0x09, 0x09};
    assert(remove_padding(data4, 5) == 5);
    
    printf("remove_padding test passed\n");
}

int main() {
    test_decrypt_blocks();
    test_remove_padding();
    return 0;
}