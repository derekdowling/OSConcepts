#include <stdio.h>
#include <stdlib.h>

#include "../minunit.h"
#include "memchunk.h"
#include "test.h"

int tests_run = 0;

int main(int argc, char **argv)
{
    char *result = all_tests();
    if (result != 0) {
        printf("%s\n", result);
    }
    else {
        printf("ALL TESTS PASSED\n");
    }
    printf("Tests run: %d\n", tests_run);

    return result != 0;
}

/**
 * Declare all tests to run here.
 */
static char * all_tests()
{
    mu_run_test(test_main);
    return 0;
}

/**
 * Write tests below.
 */

static char * test_main()
{
    int size = 3;
    struct memchunk* chunk_list;
    chunk_list = malloc(sizeof(memchunk) * size);

    int count = get_mem_layout(chunk_list, size);
    mu_assert("err msg", foo == 3);
    return 0;
}
