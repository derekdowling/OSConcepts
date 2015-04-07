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
void get_max_process_allocations();
int** get_2d_int(int x, int y);
int random_int(int max);

/* Exit Handlers */
void signal_handler(int signal);
void free_allocations();

/* Simulation Related Functions */
void simulate();
void perform_bankers(int process, int previous);
void remove_resources(int process);

/* Pretty Printers */
void output_snapshot();
void print_table(int** table);
void print_resource_table(int* table);

/* CONSTANTS */
int resource_count, process_count;
int *available_resources, *max_resources, *available_resources, *process_waiting;
int **max_process_allocations, **process_allocations, ** process_requested_allocations;

int main(void)
{
    /* Input Resources */
    resource_count = get_count_input("Number of different resource types");
    available_resources = malloc(resource_count * sizeof(int));

    get_resource_allocation(
        "Number of instances of each resource type",
        available_resources
    );

    /* Input Processes */
    process_count = get_count_input("Number of processes");
    get_max_process_allocations();

    /* Now that all inputs are provided, simulate */
    simulate();

    return 0;
}

void simulate()
{
    process_waiting = malloc(process_count * sizeof(int));

    srand(time(NULL));

    /* allocate various extra trackers */
    process_allocations = get_2d_int(process_count, resource_count);
    process_requested_allocations = get_2d_int(process_count, resource_count);

    /* Set all processes to not waiting */
    for (int p = 0; p < process_count; p++)
    {
        process_waiting[p] = 0;
    }

    /* Make a copy of max resources so we don't overwrite that value */
    max_resources = malloc(resource_count * sizeof(int));
    for (int r = 0; r < resource_count; r++)
    {
        max_resources[r] = available_resources[r];
    }

    /* CTRL+C Handling */
    signal(SIGINT, signal_handler);

    printf("\n\n**STARTING SIMULATION**\n\n");
    output_snapshot();

    /* Simulate */
    while (1)
    {
        /* sleep/increment step */
        sleep(0);

        /* Make sure we're not all sleeping */
        int all_waiting = 1;
        for (int p = 0; p < process_count; p++)
        {
            if (process_waiting[p] == 0)
            {
                all_waiting = 0;
            }
        }

        if (all_waiting == 1)
        {
            printf("All processes are in a waiting state, exiting.\n");
            exit(0);
        }

        for (int p = 0; p < process_count; p++)
        {
            /* Do nothing if process is waiting for resources */
            if (process_waiting[p] == 1)
            {
                continue;
            }

            /* Generate a process action between 0-2 */
            int action = random_int(2);

            /* Keep Executing With Given Resources */
            if (action == 0)
            {
                continue;
            }

            /* Ask For More Resources */
            else if (action == 1)
            {
                output_snapshot();

                /* Generate additional resources to request */
                for (int r = 0; r < resource_count; r++)
                {
                    int max = max_process_allocations[p][r] - process_allocations[p][r];
                    int val = 0;
                    if (max > 0)
                    {
                        val = random_int(max);
                    }
                    process_requested_allocations[p][r] = val;
                }

                /* Print Request */
                printf("Request (");
                for (int r = 0; r < resource_count; r++)
                {
                    int val = process_requested_allocations[p][r];
                    if (r + 1 < resource_count)
                    {
                        printf("%d,", val);
                    }
                    else
                    {
                        printf("%d) came from P%d\n\n", val, p + 1);
                    }
                }

                perform_bankers(p, 0);
                output_snapshot();
            }

            /* Release Resources */
            else if(action == 2)
            {
                remove_resources(p);
                output_snapshot();
            }
        }
    }
}

void remove_resources(int process)
{
    /* Tally Resources So We Leave One Running */
    int count = 0;
    for (int r = 0; r < resource_count; r++)
    {
        count += process_allocations[process][r];
    }

    /* Now start removing resources from process */
    int* released = malloc(resource_count * sizeof(int));
    for (int r = 0; r < resource_count; r++)
    {
        if (count < 2)
        {
            break;
        }

        int max = process_allocations[process][r];
        int remove = 0;

        if (max == count)
        {
            remove = random_int(max - 1);
        }
        else if (max > 0)
        {
            remove = random_int(max);
        }

        /* Deallocated first */
        process_allocations[process][r] = process_allocations[process][r] - remove;

        /* Put Resources Back In Pool */
        available_resources[r] =
            available_resources[r] + remove;

        released[r] = remove;

        count -= remove;
    }

    /* Print Released */
    printf("P%d has released (", process + 1);
    for (int r = 0; r < resource_count; r++)
    {
        int val = released[r];
        if (r + 1 < resource_count)
        {
            printf("%d,", val);
        }
        else
        {
            printf("%d) resources\n\n", val);
        }
    }

    free(released);


    /* Perform Bankers */
    for (int pro = 0; pro < process_count; pro++)
    {
        /* Only check waiting processes */
        if (process_waiting[pro] == 1)
        {
            perform_bankers(pro, 1);
        }
    }
}

void perform_bankers(int process, int previous)
{
    /* See if we can fulfill request */
    int result = 1;
    for (int r = 0; r < resource_count; r++)
    {
        if (process_requested_allocations[process][r] > available_resources[r]) {
            result = 0;
        }
    }

    /* If failed, log and make process wait */
    if (result == 0)
    {
        process_waiting[process] = 1;
        /* Print Request */
        printf("Request (");
        for (int r = 0; r < resource_count; r++)
        {
            int val = process_requested_allocations[process][r];
            if (r + 1 < resource_count)
            {
                printf("%d,", val);
            }
            else
            {
                printf(
                    "%d) from P%d cannot be satisfied, P%d is in waiting state\n\n",
                    val,
                    process + 1,
                    process + 1
                );
            }
        }
    }
    /* Otherwise, allocate */
    else
    {
        process_waiting[process] = 0;

        /* Print Request */
        if (previous == 0)
        {
            printf("Request (");
        }
        else
        {
            printf("Previous request of (");
        }
        for (int r = 0; r < resource_count; r++)
        {
            int val = process_requested_allocations[process][r];
            if (r + 1 < resource_count)
            {
                printf("%d,", val);
            }
            else
            {
                if (previous == 0)
                {
                    printf("%d) from P%d has been granted\n\n", val, process + 1);
                }
                else
                {
                    printf("%d) from P%d has been satisfied\n\n", val, process + 1);
                }
            }
        }

        /* Updated counters */
        for (int r = 0; r < resource_count; r++)
        {
            /* Remove From Available */
            available_resources[r] =
                available_resources[r] -
                process_requested_allocations[process][r];

            /* Allocate */
            process_allocations[process][r] = 
                process_allocations[process][r] +
                process_requested_allocations[process][r];

            /* Fulfil request */
            process_requested_allocations[process][r] = 0;
        }
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

/* Generate num between 0 and max additional allowable value */
int random_int(int max)
{
    if (max == 0)
    {
        printf("WTF\n");
        exit(-1);
    }
    return (rand() % max) + 1;
}

void output_snapshot()
{
    printf("Current snapshot:\n\n");

    printf("Current Allocation:\n");
    print_table(process_allocations);

    printf("\nCurrent Request:\n");
    print_table(process_requested_allocations);

    printf("\nCurrently Available Resources:\n");
    print_resource_table(available_resources);

    printf("\nMaximum Possible Request:\n");
    print_table(max_process_allocations);

    printf("\nMaximum Available Resources:\n");
    print_resource_table(max_resources);
    printf("\n");
}

/* Nasty table printing function */
void print_table(int** table)
{
    /* Print Rows */
    for (int x = 0; x < process_count; x++)
    {
        printf("P%d", x + 1);
        for (int y = 0; y < resource_count; y++)
        {
            printf(" %d", table[x][y]);
        }
        printf("\n");
    }
}

void print_resource_table(int* table)
{
    /* Print Colums */
    for (int x = 0; x < resource_count; x++)
    {
        printf("%d ", table[x]);
    }
    printf("\n");
}

/* Exits the simulation when CTRL+C is pressed */
void signal_handler(int signal)
{
    printf("Simulation has been ended.\n");
    exit(0);
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
void get_max_process_allocations()
{
    max_process_allocations = get_2d_int(process_count, resource_count);

    /* build input allocations */
    for (int p = 0; p < process_count; p++)
    {
        char input[64];
        int i = 0;

        /* get input */
        printf("\nDetails of P%d (%d Resources): ", p + 1, resource_count);
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

            max_process_allocations[p][i] = atoi(token);
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
