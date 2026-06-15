#ifndef CORE_MEMORY_MEMORY_H
#define CORE_MEMORY_MEMORY_H

#include <stdio.h>

#define MEMORY_FILE "memory.bin"
#define MEMORY_SIZE 1024

int memory_init();
int memory_alloc(int size, int *base_out);
int memory_write(int addr, int value);
int memory_read(int addr, int *value_out);

#endif
