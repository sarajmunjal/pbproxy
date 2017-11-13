#include <openssl/evp.h>
#include <openssl/aes.h>
#include <openssl/err.h>
#include <string.h>

#define BLOCK_SIZE 16

size_t get_ct_size(unsigned int d) {
    if (d < 0) {
        return 0;
    }
    if (d % 16) {
        return ((d / 16) + 1) * 16;
    }
    return d;
}


void handle_errors(void) {
    unsigned long errCode;

    fprintf(stderr, "An error occurred in encryption\n");
    while (errCode = ERR_get_error()) {
        char *err = ERR_error_string(errCode, NULL);
        fprintf(stderr, "%s\n", err);
    }
    abort();
}

int encrypt(unsigned char *plaintext, int plaintext_len, unsigned char *key,
            unsigned char *iv, unsigned char *ciphertext) {
    EVP_CIPHER_CTX *ctx;

    int len;

    int ciphertext_len;

    /* Create and initialise the context */
    if (!(ctx = EVP_CIPHER_CTX_new())) handle_errors();

    /* Initialise the encryption operation. IMPORTANT - ensure you use a key
     * and IV size appropriate for your cipher
     * In this example we are using 256 bit AES (i.e. a 256 bit key). The
     * IV size for *most* modes is the same as the block size. For AES this
     * is 128 bits */
    if (1 != EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv))
        handle_errors();

    /* Provide the message to be encrypted, and obtain the encrypted output.
     * EVP_EncryptUpdate can be called multiple times if necessary
     */
    if (1 != EVP_EncryptUpdate(ctx, ciphertext, &len, plaintext, plaintext_len))
        handle_errors();
    ciphertext_len = len;

    /* Finalise the encryption. Further ciphertext bytes may be written at
     * this stage.
     */
    if (1 != EVP_EncryptFinal_ex(ctx, ciphertext + len, &len)) handle_errors();
    ciphertext_len += len;

    /* Clean up */
    EVP_CIPHER_CTX_free(ctx);

    return ciphertext_len;
}

int decrypt(unsigned char *ciphertext, int ciphertext_len, unsigned char *key,
            unsigned char *iv, unsigned char *plaintext) {
    EVP_CIPHER_CTX *ctx;

    int len;

    int plaintext_len;

    /* Create and initialise the context */
    if (!(ctx = EVP_CIPHER_CTX_new())) handle_errors();

    /* Initialise the decryption operation. IMPORTANT - ensure you use a key
     * and IV size appropriate for your cipher
     * In this example we are using 256 bit AES (i.e. a 256 bit key). The
     * IV size for *most* modes is the same as the block size. For AES this
     * is 128 bits */
    if (1 != EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv))
        handle_errors();

    /* Provide the message to be decrypted, and obtain the plaintext output.
     * EVP_DecryptUpdate can be called multiple times if necessary
     */
    if (1 != EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, ciphertext_len))
        handle_errors();
    plaintext_len = len;

    /* Finalise the decryption. Further plaintext bytes may be written at
     * this stage.
     */
    if (1 != EVP_DecryptFinal_ex(ctx, plaintext + len, &len)) handle_errors();
    plaintext_len += len;

    /* Clean up */
    EVP_CIPHER_CTX_free(ctx);

    return plaintext_len;
}

void init_crypto() {
    ERR_load_crypto_strings();
    OpenSSL_add_all_algorithms();
    OPENSSL_config(NULL);
}

void clean_up_crypto() {
    EVP_cleanup();
    ERR_free_strings();
}
//int main(void) {
//    /* Set up the key and iv. Do I need to say to not hard code these in a
//     * real application? :-)
//     */
//
//    /* A 256 bit key */
//    unsigned char *key = (unsigned char *) "01234567890123456789012345678901";
//
//    /* A 128 bit IV */
//    unsigned char *iv = (unsigned char *) "0123456789012345";
//
//    /* Message to be encrypted */
//    unsigned char *plaintext =
//            (unsigned char *) "The quick brown fox jumps over the lazy dog";
//
//    /* Buffer for ciphertext. Ensure the buffer is long enough for the
//     * ciphertext which may be longer than the plaintext, dependant on the
//     * algorithm and mode
//     */
//    unsigned char ciphertext[128];
//
//    /* Buffer for the decrypted text */
//    unsigned char decryptedtext[128];
//
//    int decryptedtext_len, ciphertext_len;
//
//    /* Initialise the library */
//
//
//    /* Encrypt the plaintext */
//    ciphertext_len = encrypt(plaintext, strlen((char *) plaintext), key, iv,
//                             ciphertext);
//
//    /* Do something useful with the ciphertext here */
//    printf("Ciphertext is:\n");
//    BIO_dump_fp(stdout, (const char *) ciphertext, ciphertext_len);
//
//    /* Decrypt the ciphertext */
//    decryptedtext_len = decrypt(ciphertext, ciphertext_len, key, iv,
//                                decryptedtext);
//
//    /* Add a NULL terminator. We are expecting printable text */
//    decryptedtext[decryptedtext_len] = '\0';
//
//    /* Show the decrypted text */
//    printf("Decrypted text is:\n");
//    printf("%s\n", decryptedtext);
//
//    /* Clean up */
//    EVP_cleanup();
//    ERR_free_strings();
//
//    return 0;
//}
