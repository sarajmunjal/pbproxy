#include <stdio.h>
#include "../src/ncrypto.c"

int main() {
//    init_crypto();
    unsigned char *key = (unsigned char *) "0123456789012345";
    unsigned char *iv = (unsigned char *) "01234567";

    unsigned char *plaintext =
            (unsigned char *) "The quick brown fox jumps over the lazy dog";

    /* Buffer for ciphertext. Ensure the buffer is long enough for the
     * ciphertext which may be longer than the plaintext, dependant on the
     * algorithm and mode
     */
    unsigned char ciphertext[strlen(plaintext)];

    /* Buffer for the decrypted text */
    unsigned char decryptedtext[strlen(plaintext) + 1];
    encrypt(plaintext, strlen((char *) plaintext), key, iv,
            ciphertext);
    printf("Ciphertext is:\n");
    BIO_dump_fp(stdout, (const char *) ciphertext, strlen(plaintext));

    /* Decrypt the ciphertext */
    decrypt(ciphertext, strlen(plaintext), key, iv, decryptedtext);

    /* Add a NULL terminator. We are expecting printable text */
    decryptedtext[strlen(plaintext)] = '\0';

    /* Show the decrypted text */
    printf("Decrypted text is:\n");
    printf("%s\n", decryptedtext);
//    clean_up_crypto();
    return 0;
}