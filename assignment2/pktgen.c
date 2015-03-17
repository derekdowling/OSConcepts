/**
 * A UDP based random "Packet" streamer.
 */
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <resolv.h>
#include <stdlib.h>
#include <signal.h>
#include <arpa/inet.h>

#define MAXBUF		    1024
#define MAX_TTL		    4
#define MAX_PAYLOAD_INDEX   4
#define MAX_ROUTE_INDEX     3

/* Packet Generator Functions */
int rand_limit_floor(int floor, int limit);
int rand_limit(int limit);
void generate_packet(char* raw_packet);
void signal_handler(int signal);
void update_statistics();
void increment_stats(int src, int dest);
int build_socket(int port);

char** ROUTERS = (char* []) {
    "192.168.128.0",
    "192.168.192.0",
    "192.224.0.0",
    "168.130.192.01"
};

char** PAYLOADS = (char* []) {
    "\"Hello!\"",
    "What's your name?",
    "Testing Test file.",
    "I am a longer test string being sent in a packet as the payload, seeeeeeeeeee!",
    ""
};

typedef struct {
    int a_to_b;
    int a_to_c;
    int b_to_a;
    int b_to_c;
    int c_to_a;
    int c_to_b;
    int invalid;
} Stats;

Stats STATS = {0, 0, 0, 0, 0, 0, 0};
static int packet_id_counter = 0;
static int keep_going = 1;
FILE* stats_file;

int main (int argc, char *argv[])
{
    int socketfd, port, counter;
    struct sockaddr_in dest;
    char buffer[MAXBUF];
    char* packet_file_path;

    if (argc != 3)
    {
        fprintf(
            stderr,
            "Invalid arg count, should be: <port number to connect to router> <packets file path>"
        );
        exit(-1);
    }

    // parse args
    port = atoi(argv[1]);
    packet_file_path = argv[2];

    stats_file = fopen(packet_file_path, "w");
    if (stats_file == NULL)
    {
	fprintf(stderr, "Error opening packet stats file.\n");
	exit(-1);
    }

    // build our socket
    /* socketfd = build_socket(port); */

    socketfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    dest.sin_family = AF_INET;
    dest.sin_port = htons(port);
    dest.sin_addr.s_addr = INADDR_ANY;

    signal(SIGINT, signal_handler);

    counter = 0;
    while(keep_going)
    {
	bzero(buffer, MAXBUF);
	generate_packet(buffer);

	if (sendto(
	    socketfd,
	    buffer,
	    strlen(buffer) + 1,
	    0,
	    (struct sockaddr*) &dest,
	    sizeof(dest)) != -1
	) {
	    counter++;
	    if (counter == 20)
	    {
		update_statistics();
		counter = 0;
	    }

	    // sleep for two seconds as per spec as not to flood the router
	    sleep(2);
	}
    }

    // now tear everything back down
    close(socketfd);
    fclose(stats_file);

    return 0;
}

/**
 * Outputs updated statistics to file.
 */
void update_statistics()
{
    // Ugly over width violation, but no nicer way to do this
    fprintf(stats_file,
        "NetA to NetB: %d\nNetA to NetC: %d\nNetB to NetA: %d\nNetB to NetC: %d\nNetC to NetA: %d\nNetC to NetB: %d\nInvalid Destination: %d\n",
        STATS.a_to_b,
        STATS.a_to_c,
        STATS.b_to_a,
        STATS.b_to_c,
        STATS.c_to_a,
        STATS.c_to_b,
        STATS.invalid
    );
    rewind(stats_file);
    printf("Updating generation statistics.\n");
}

/**
 * Generates a new packet and places it in the specified buffer.
 */
void generate_packet(char* raw_packet)
{
    int src, dest, ttl, payload;

    ttl = rand_limit_floor(1, MAX_TTL);
    src = rand_limit(MAX_ROUTE_INDEX);
    dest = rand_limit(MAX_ROUTE_INDEX);
    payload = rand_limit(MAX_ROUTE_INDEX);

    // ensure src != dest for addresses and we arent sending from an invalid
    // src address
    while (src == dest || src == MAX_ROUTE_INDEX)
    {
        src = rand_limit(MAX_ROUTE_INDEX);
    }

    // offload updating our global stats struct
    increment_stats(src, dest);

    sprintf(
        raw_packet,
        "%d, %s, %s, %d, %s",
        packet_id_counter,
        ROUTERS[src],
        ROUTERS[dest],
        ttl,
        PAYLOADS[payload]
    );

    // increment id counter
    packet_id_counter++;
}

/**
 * This handles updating our stats. Because we are manually setting very
 * specific cases, there is a lot of gross case checking here.
 */
void increment_stats(int src, int dest)
{
    // handle invalid case first
    if (dest == MAX_ROUTE_INDEX) {
        STATS.invalid = STATS.invalid + 1;
    }
    // we should never have src == dest based on how packets are generated
    else if (src == 0)
    {
        if (dest == 1)
        {
            STATS.a_to_b = STATS.a_to_b + 1;
        }
        else
        {
            STATS.a_to_c = STATS.a_to_c + 1;
        }
    }
    else if(src == 1)
    {
        if (dest == 0)
        {
            STATS.b_to_a = STATS.b_to_a + 1;
        }
        else
        {
            STATS.b_to_c = STATS.b_to_c + 1;
        }
    }
    else if (src == 2)
    {
        if (dest == 0)
        {
            STATS.c_to_a = STATS.c_to_a + 1;
        }
        else
        {
            STATS.c_to_b = STATS.c_to_b + 1;
        }
    }
}

/**
 * Returns a random int between floor and limit inclusive.
 */
int rand_limit_floor(int floor, int limit)
{
    return floor + rand_limit(limit - floor);
}

/**
 * Return a random number between 0 and limit inclusive.
 */
int rand_limit(int limit)
{

    int divisor = RAND_MAX / ( limit + 1);
    int retval;

    do {
        retval = rand() / divisor;
    } while (retval > limit);

    return retval;
}

void signal_handler(int signal)
{
    update_statistics();
    printf("Terminating...");
    keep_going = 0;
}

/**
 * Handles building the socket, binding, and listening it to the specified
 * port.
 */
int build_socket(int port)
{
    struct sockaddr_in sock_addr;
    int socketfd;
    
    if ((socketfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
    {
	fprintf(stderr, "Unable to create a new socket.");
	exit(errno);
    }

    // Specify socket parameters
    memset((char *) &sock_addr, 0, sizeof(sock_addr));
    sock_addr.sin_family = AF_INET;
    sock_addr.sin_addr.s_addr = INADDR_ANY;
    sock_addr.sin_port = htons(9999);

    // bind to specified socket
    if (bind(socketfd, (struct sockaddr *) &sock_addr, sizeof(sock_addr)) == -1)
    {
	fprintf(stderr, "Error binding socket on port %d\n", port);
	exit(errno);
    }

    return socketfd;
}
