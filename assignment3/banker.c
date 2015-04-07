/* Responsible for implementing Banker's Algorithm */

#include <unistd.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>

/* Simulation Inputs */
int get_count_input(char* msg);
void get_resource_allocation(char* msg, int* array);
int get_resource();
void get_process_allocations();
int** get_2d_int(int x, int y);

/* Exit Handlers */
void signal_handler(int signal);
void free_allocations();

/* Simulation Related Functions */
void run_bankers(
    int resource_count,
    int process_count,
    int* resource_allocations,
    int** process_allocations
);

void next_step(
    int process_count,
    int** process_allocations,
    int** max_process_allocations
);

void output_snapshot(
    int process_count,
    int resource_count,
    int** process_allocations,
    int** process_requested_allocations,
    int* available_resources,
    int** max_process_allocations,
    int* max_resources
);

int generate_rand_inc(int current, int max);

/* Pretty Printers */
void print_table(int process_count, int resource_count, int** table);
void print_resource_table(int resource_count, int* table);

/* CONSTANTS */
const int step_multiplier = 5;
int resource_count, process_count;
int* resource_allocations;
int** process_allocations;

int main(void)
{
    /* Input Resources */
    resource_count = get_count_input("Number of different resource types");
    resource_allocations = malloc(resource_count * sizeof(int));

    get_resource_allocation(
        "Number of instances of each resource type",
        resource_allocations
    );

    /* Input Processes */
    process_count = get_count_input("Number of processes");
    get_process_allocations();

    signal(SIGINT, signal_handler);

    /* Now that all inputs are provided, simulate */
    run_bankers(
        resource_count,
        process_count,
        resource_allocations,
        process_allocations
    );

    return 0;
}

void run_bankers(
    int resource_count,
    int process_count,
    int* max_resources,
    int** max_process_allocations
) {

    int step;
    int** process_allocations, **process_requested_allocations;
    int available_resources[resource_count];

    step = 1;
    srand(time(NULL));

    /* allocate various extra trackers */
    process_allocations = get_2d_int(process_count, resource_count);
    process_requested_allocations = get_2d_int(process_count, resource_count);

    for (int r = 0; r < resource_count; r++)
    {
        available_resources[r] = max_resources[r];
    }

    output_snapshot(
        process_count,
        resource_count,
        process_allocations,
        process_requested_allocations,
        available_resources,
        max_process_allocations,
        max_resources
    );

    return;

    /* Simulate */
    while (1)
    {
        if (step % step_multiplier == 0)
        {
            for (int p = 0; p < process_count; p++)
            {
                /* Generate a process action between 0-2 */
                int r = rand() % 3;

                /* Keep Executing */
                if (r == 0)
                {
                    continue;
                }

                /* Ask For More Resources */
                else if (r == 1)
                {
                    for (int r = 0; r < resource_count; r++)
                    {
                        process_allocations[p][r] =
                            generate_rand_inc(
                                process_allocations[p][r],
                                max_process_allocations[p][r]
                            );
                    }
                    // TODO: print snapshot
                    // TODO: check availibility or wait
                }

                /* Release Resources */
                else
                {
                    for (int r = 0; r < resource_count; r++)
                    {
                        int allocated = process_allocations[p][r];

                        /* Deallocated first */
                        process_allocations[p][r] = 0;

                        /* Put Resources Back In Pool */
                        available_resources[r] =
                            available_resources[r] + allocated;
                    }

                    // TODO: reprocess waiters
                }
            }
        }

        /* sleep/increment step */
        sleep(1);
        step++;
    }
}

int** get_2d_int(int x, int y)
{
    int** array = malloc(x * sizeof(int*));

    for (int i = 0; i < x; i++)
    {
        array[i] = malloc(y * sizeof(int));
    }

    return array;
}

int generate_rand_inc(int current, int max)
{
    if (current == max)
    {
        return 0;
    }
    else
    {
        /* Generate num between 0 and max additional allowable value */
        return (rand() % (max - current + 1));
    }
}

void output_snapshot(
    int process_count,
    int resource_count,
    int** process_allocations,
    int** process_requested_allocations,
    int* available_resources,
    int** max_process_allocations,
    int* max_resources
) {
    printf("Current Allocation:\n");
    print_table(process_count, resource_count, process_allocations);
    printf("\nCurrent Request:\n");
    print_table(process_count, resource_count, process_allocations);
    printf("\nCurrently Available Resources:\n");
    print_resource_table(resource_count, available_resources);
    printf("\nMaximum Possible Request:\n");
    print_table(process_count, resource_count, process_allocations);
    printf("\nMaximum Available Resources:\n");
    print_resource_table(resource_count, max_resources);
    printf("\n");
}

/* Nasty table printing function */
void print_table(int process_count, int resource_count, int** table)
{
    /* Print Rows */
    for (int x = 0; x < process_count; x++)
    {
        printf("P%d", x);
        for (int y = 0; y < resource_count; y++)
        {
            printf(" %d", y);
        }
        printf("\n");
    }
}

void print_resource_table(int resource_count, int* table)
{
    /* Print Colums */
    for (int x = 0; x < resource_count; x++)
    {
        printf(" %d", x);
    }
}

/* Exits the simulation when CTRL+C is pressed */
void signal_handler(int signal)
{
    printf("Simulation has been ended.\n");
    free_allocations();
    exit(0);
}

/* frees all global resources */
void free_allocations()
{
    free(resource_allocations);

    for (int p = 0; p < process_count; p++)
    {
        free(process_allocations[p]);
    }
    free(process_allocations);

    return;
}

/* Asks the user for input using the provided prompt */
int get_count_input(char* msg)
{
    char buffer[64];

    /* read stdin */
    printf("%s: ", msg);
    fflush(stdout);
    fgets(buffer, 64, stdin);

    /* convert string to int */
    return atoi(buffer);
}

/* Handles building a 2D process allocation array. */
void get_process_allocations()
{
    process_allocations = get_2d_int(process_count, resource_count);

    /* build input allocations */
    for (int p = 0; p < process_count; p++)
    {
        char input[64];
        int i = 0;

        /* get input */
        printf("\nDetails of P%d (%d Resources): ", p, resource_count);
        fflush(stdout);
        fgets(input, 64, stdin);

        char* token = strtok(input, " ");
        while(token != NULL) {

            if (i == resource_count) {
                fprintf(
                    stderr,
                    "Too many inputs for expected count: %d. Exiting.\n",
                    resource_count
                );
                exit(-1);
            }

            process_allocations[p][i] = atoi(token);
            token = strtok(NULL, " ");
            i++;
        }
    }
}

/* Builds a 1D allocation array from input. */
void get_resource_allocation(char* msg, int* array)
{
    char input[64];
    int i = 0;

    /* get input */
    printf("\n%s (%d Resources): ", msg, resource_count);
    fflush(stdout);
    fgets(input, 64, stdin);
    // TODO: remove
    puts(input);

    char* token = strtok(input, " ");
    while(token != NULL) {

        if (i == resource_count) {
            fprintf(
                stderr,
                "Too many inputs for expected count: %d. Exiting.\n",
                resource_count
            );
            free(array);
            exit(-1);
        }

        array[i] = atoi(token);
        token = strtok(NULL, " ");
        i++;
    }
}
