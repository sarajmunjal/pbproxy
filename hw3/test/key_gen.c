#include <stdlib.h>
#include <time.h>
#include <stdio.h>

size_t get_ct_size(int d) {
    int rem = d % 16;
    if (rem == 0) {
        return d;
    }
    // round_up to nearest 16 if not divisible
    return d + (16 - rem);
}

unsigned char *gen_rdm_bytestream(size_t num_bytes) {
    unsigned char *stream = malloc(num_bytes + 1);
    size_t i;

    for (i = 0; i < num_bytes; i++) {
        stream[i] = rand();
    }
    stream[i] = '\0';
    return stream;
}

int main() {
    srand((unsigned int) time(NULL));
    FILE *outf = fopen("./mykey", "w+");
    printf("%d %d", get_ct_size(256), get_ct_size(1024));
    char *str = gen_rdm_bytestream(16);
    for (int i = 0; i < 32; i++) {
        fprintf(outf, "%02X ", str[i] & 0xFF);
    }
    return 0;
}