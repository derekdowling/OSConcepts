#include <stdio.h>
#include <stdlib.h>
#include "memchunk.h"

int main(int argc, char **argv)
{
    printf("Scanning...\n");
    int size = 16;

    // ALlocate a memchunk array
    struct memchunk* chunk_list;
    chunk_list = malloc(sizeof(struct memchunk) * size);

    int count = get_mem_layout(chunk_list, size);
    printf("\nUnique Chunks: %d\n", count);

    for (int i = 0; i < size; i++) {
        struct memchunk chunk = chunk_list[i];
        printf(
            "Start: %p, Size: %lu, RW: %d\n", chunk.start, chunk.length, chunk.RW
        );
    }

    free(chunk_list);

    return 0;
}
