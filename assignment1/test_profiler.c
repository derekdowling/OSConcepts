#include <stdio.h>
#include "../minunit.h"

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
    mu_run_test(test_foo);
    return 0;
}

/**
 * Write tests below.
 */

static char * test_foo()
{
    mu_assert("err msg", foo == 3);
    return 0;
}
