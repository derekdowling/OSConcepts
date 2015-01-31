#ifndef MEMCHUNK_H_
#define MEMCHUNK_H_

#include <stdint.h>

struct memchunk {
    void *start;
    unsigned long length;
    int RW;
};

int get_mem_layout(struct memchunk * chunk_list, int size);
int get_page_permission(uintptr_t current_addr, int page_size);
void sigsegv_handler(int sig);

#endif
