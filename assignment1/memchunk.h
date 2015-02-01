#ifndef MEMCHUNK_H_
#define MEMCHUNK_H_

#include <stdint.h>

struct memchunk {
    void *start;
    unsigned long length;
    int RW;
};

int get_mem_layout(struct memchunk * chunk_list, int size);
int get_rw(char* current_addr);
int can_read(char* current_addr);
int can_write(char* current_addr);
void sigsegv_handler(int sig);

#endif
