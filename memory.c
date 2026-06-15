#include "core/memory/memory.h"
#include <stdlib.h>

static int next_free = 0;

int memory_init() {
    FILE *fp = fopen(MEMORY_FILE, "wb");
    if (!fp) return -1;

    char zero = 0;
    for (int i = 0; i < MEMORY_SIZE; i++) {
        fwrite(&zero, 1, 1, fp);
    }

    fclose(fp);
    next_free = 0;
    return 0;
}

int memory_alloc(int size, int *base_out) {
    if (next_free + size > MEMORY_SIZE) return -1;

    *base_out = next_free;
    next_free += size;
    return 0;
}

int memory_write(int addr, int value) {
    FILE *fp = fopen(MEMORY_FILE, "rb+");
    if (!fp) return -1;

    fseek(fp, addr, SEEK_SET);
    fwrite(&value, sizeof(int), 1, fp);

    fclose(fp);
    return 0;
}

int memory_read(int addr, int *value_out) {
    FILE *fp = fopen(MEMORY_FILE, "rb");
    if (!fp) return -1;

    fseek(fp, addr, SEEK_SET);
    fread(value_out, sizeof(int), 1, fp);

    fclose(fp);
    return 0;
}
