// Profile the programs main memory

#include <stdlib.h>
#include <limits.h>
#include <unistd.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdint.h>
#include <strings.h>

#include "memchunk.h"

// Sets our current memchunk for signhandler use
struct memchunk current_chunk;
static jmp_buf env;

/**
 * Parses and groups all consecutive memory chunks based on their "RW" struct
 * attributes.
 *
 * @return total number of consecutively grouped read/write permissions memory
 * chunks
 */
int get_mem_layout(struct memchunk * chunk_list, int size)
{
    /* // set base counters and such */
    int page_size = getpagesize();
    printf("Page length: %d\n", page_size);
    unsigned long max_memory_addr = ULONG_MAX / page_size;
    int total_chunks = 0;
    /* int list_size = 0; */

    /* // use an outrageous number so we immediately start a new chunk */
    uintptr_t current_addr = 0;
    int current_permission;
    int last_permission;
    /* struct memchunk chunk = {0}; */

    /* // set our initial permission so we don't double count the first chunk */
    last_permission = get_page_permission(current_addr, page_size);
    current_addr += page_size;

    /* // iterate through all memchunks */
    for (
        current_addr = 0; current_addr < max_memory_addr; current_addr += page_size
    ) {

        printf("\nAddress: %lu", current_addr);
        current_permission = get_page_permission(current_addr, page_size);
        printf(" Permission: %d", current_permission);

        /* // If our latest chunk has a different access mode, increment and */
        /* // update */
        /* if (current_permission != last_permission) { */
            /* total_chunks++; */
            /* last_permission = current_permission; */

            /* if (list_size < size) { */
                /* chunk_list[list_size] = chunk; */
                /* memset(&chunk, 0, sizeof(chunk)); */
                /* chunk.start = (void*) current_addr; */
                /* chunk.RW = current_permission; */
            /* } */
        /* } */

        /* // Otherwise, update our memchunk */
        /* else { */
            /* chunk.length += page_size; */
        /* } */
    }

    return total_chunks;
}

/**
 * Tests a page in memory to see what it's user level permissions are.
 */
int get_page_permission(uintptr_t current_addr, int page_size)
{
    char * block = (char*) current_addr;

    // set our signal handler
    (void) signal(SIGSEGV, sigsegv_handler);

    // Check read permissions
    if (!setjmp(env)) {

        // see if we seg fault on read attempt
        char temp = *block;

        // make sure we write back the old value if successful so we don't
        // overwrite this programs own memory
        if (!setjmp(env)) {

            *block = 'a';
            *block = temp;
            return 1;

        } else {
            return 0;
        }

    } else {
        return -1;
    }

    // should never make it here, but this squashes warnings
    return 0;
}

/**
 * Handles any Seg Fault Signals caused by accessing invalid memory
 * within the app.
 */
void sigsegv_handler(int sig)
{
    printf("... Seg Fault!");
    longjmp(env, 1);
}

int main(int argc, char **argv)
{
    printf("Scanning...\n");
    int size = 3;

    // ALlocate a memchunk array
    struct memchunk* chunk_list;
    chunk_list = malloc(sizeof(struct memchunk*) * size);

    int count = get_mem_layout(chunk_list, size);
    printf("Unique Chunks: %d\n", count);

    return 0;
}
