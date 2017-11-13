#include <string.h>
#include <openssl/aes.h>
#include <openssl/rand.h>
#include <openssl/hmac.h>
#include <openssl/buffer.h>

struct ctr_state {
    unsigned char ivec[16];  /* ivec[0..7] is the IV, ivec[8..15] is the big-endian counter */
    unsigned int num;
    unsigned char ecount[16];
};

int init_ctr(struct ctr_state *state, const unsigned char *iv) {
    /* aes_ctr128_encrypt requires 'num' and 'ecount' set to zero on the
     * first call. */
    state->num = 0;
    memset(state->ecount, 0, 16);

    /* Initialise counter in 'ivec' to 0 */
    memset(state->ivec + 8, 0, 8);

    /* Copy IV into 'ivec' */
    memcpy(state->ivec, iv, 8);
}

void encrypt(void *data, size_t data_len, char *enc_key, char *iv, void *buf) {
    AES_KEY key;
    struct ctr_state state;
    if (AES_set_encrypt_key(enc_key, 128, &key) < 0) {
        fprintf(stderr, "Could not set encryption key.");
        exit(1);
    }
    init_ctr(&state, iv);
    AES_ctr128_encrypt(data, buf, data_len, &key, state.ivec, state.ecount, &state.num);
}

void decrypt(void *cipher_text, size_t ct_size, char *enc_key, char *iv, void *buf) {
    AES_KEY key;
    struct ctr_state state;
    if (AES_set_encrypt_key(enc_key, 128, &key) < 0) {
        fprintf(stderr, "Could not set encryption key.");
        exit(1);
    }
    init_ctr(&state, iv);
    AES_ctr128_encrypt(cipher_text, buf, ct_size, &key, state.ivec, state.ecount, &state.num);
}