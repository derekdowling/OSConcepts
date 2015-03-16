#include <stdio.h>
#include <unistd.h>

int main(int argc, char **argv)
{

    printf("Hello 1");
    fork();
    printf("Hello 2");
    fork();
    printf("Hello 3");

    return 0;
}
