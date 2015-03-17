/**
 * A very simple file server.
 */
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <limits.h>

#define CHUNK_SIZE 1024
#define IP "192.168.10.1"

/* Struct Definitions */
typedef struct {
    char* address;
    int prefix_length;
    char* next_hop;
} Router;

typedef struct {
    Router** routes;
    int size;
    int max_size;
} RouterTable;

typedef struct {
    int id;
    char* src;
    char* dest;
    int TTL;
    char* payload;
} Packet;

typedef struct {
    int expired;
    int unroutable;
    int direct;
    int router_b;
    int router_c;
} Stats;


/* Function Definitions */
RouterTable* RouterTable_new();
RouterTable* build_router_table(char* table_path);
void add_new_router(RouterTable* table, char address[16], int prefix_length, char next_hop[8]);
int build_packet(Packet* packet, char* raw_packet);
Packet Packet_new(int id, char* src, char* dest, int TTL, char* payload);
void route_packet(FILE* stats_file, RouterTable* table, char* stream);
int find_destination_router(Router* router, RouterTable* table, Packet* packet);
void output_statistics();
int build_socket(int port);
int set_server_address(RouterTable* table);
void signal_handler(int signal);
int compare_subnet(int prefix_length, char* candidate, char* destination);
uint32_t parse_ipv4_string(char* ipAddress);

/* Global Stats Struct */
Stats stats;
static int keep_running = 1;
FILE* stats_file;

int main(int argc, char *argv[])
{
	char raw_packet[MAX_BUFFER];
	struct sockaddr_in inc_socket;
	int socketfd, counter, port, sock_length = sizeof(inc_socket);

	if (argc != 4)
	{
		printf("Bad Args, should be <listening-port> <base-directory-path> <logfile-path>");
		exit(-1);
	}

	port = atoi(argv[1]);
	char* table_file_path = argv[2];
	char* stats_file_path = argv[3];

	// parse out routes into Route array pointer from RT_A.txt
	RouterTable* table = build_router_table(table_file_path);

	// initialize the statistics struct
	stats = (Stats) {0, 0, 0, 0, 0};

	stats_file = fopen(stats_file_path, "w");
	if (stats_file == NULL)
	{
		fprintf(stderr, "Error opening stats file.\n");
		exit(-1);
	}

	socketfd = build_socket(port);

    signal(SIGINT, signal_handler);

	// listen infinitely for incoming packets
	counter = 0;
	while (keep_running)
	{
		bzero(raw_packet, MAX_BUFFER);

		// Waits until we receive something
		if (recvfrom(
				socketfd,
				raw_packet,
				MAX_BUFFER,
				0,
				(struct sockaddr*) &inc_socket,
				&sock_length
			) != -1
		) {
			// route and increment counter
			route_packet(stats_file, table, raw_packet);
			counter++;
		
			// see if it's time to output statistcs
			if (counter == 20)
			{
				counter = 0;
				output_statistics();
			}
		}
		else
		{
			fprintf(stderr, "Recvfrom err#: %d\n", errno);
		}
	}

	// now tear everything back down
	close(socketfd);
	fclose(stats_file);

	for(int i = 0; i < table->size; i++)
	{
		free(table->routes[i]);
	}

	free(table);
}

void route_packet(FILE* stats_file, RouterTable* table, char* stream)
{
	Packet* packet = malloc(sizeof(Packet));
	Router* router = malloc(sizeof(Router));
	char* next_hop;

	if (build_packet(packet, stream) != 0)
	{
		stats.expired = stats.expired + 1;
		free(packet);
		return;
	}

	if (find_destination_router(router, table, packet) == -1)
	{
		stats.unroutable = stats.unroutable + 1;
	}
	else
	{
		if (strcmp(router->next_hop, "0") == 0)
		{
			stats.direct = stats.direct + 1;
			printf(
				"Delivering direct: packet ID=%d, dest=%s\n",
				packet->id,
				router->address
			);
		}
		else
		{
			// increment counters and free packet allocations
			next_hop = router->next_hop;
			if (strcmp(next_hop, "RouterB") == 0)
			{
				stats.router_b = stats.router_b + 1;
			}
			else if (strcmp(next_hop, "RouterC") == 0)
			{
				stats.router_c = stats.router_c + 1;
			}
		}

		// finally free the created packet
		free(packet);
		free(router);
	}
}

/**
 * Attempts to find the destination router for the given packet from the provided
 * table.
 * Returns a Router pointe or NULL if no matches were found.
 */
int find_destination_router(Router* router, RouterTable* table, Packet* packet)
{
	int prefix_length;

	for(int i = 0; i < table->size; i++)
	{
		Router* candidate = table->routes[i];

		// see if router and destination are in the same subnet
		if ((compare_subnet(
			candidate->prefix_length,
			candidate->address,
			packet->dest) == 0
			)  &&
			strcmp(candidate->address, packet->dest) == 0
		) {
			*router = *candidate;
			return 0;
		}
	}

	return -1;
}

/**
 * Parses out a route table struct from the provided table file path.
 */
RouterTable* build_router_table(char* table_path)
{
	char* token = NULL;
	RouterTable* table = RouterTable_new();
	FILE* table_file = fopen(table_path, "r");

	if (table_file == NULL)
	{
		perror("Can't read invalid table file path\n");
		exit(-1);
	}

	// parse line by line through the file
	while (!feof(table_file))
	{
		char address[16];
		int prefix_length;
		char next_hop[8];

		if (fscanf(table_file, "%s %d %s", address, &prefix_length, next_hop) != 3)
		{
			continue;
		}
		add_new_router(table, address, prefix_length, next_hop);
	}

	fclose(table_file);

	return table;
}

/**
 * Allocates and initializes a new RouteTable.
 */
RouterTable* RouterTable_new()
{
	RouterTable* table = malloc(sizeof(RouterTable));
	table->size = 0;
	table->max_size = 20;
	table->routes = malloc(table->max_size * sizeof(Router*));
	return table;
}

/**
 * Handles parsing, and preparing an incoming packet for:
 * <packet ID>, <source IP>, <destination IP>, <TTL>, <payload>
 *
 * If the packet can't be properly parsed or the TTL is too low, discards
 * packet and returns NULL.
 */
int build_packet(Packet* packet, char* raw_packet)
{
	int id, TTL;
	char src[16], dest[16];
	char* payload;

	// super ghetto parser, csv lists suck in c
	char selector[] = ", ";
	char* token = strtok(raw_packet, selector);
	int counter = 0;
	while (token && counter < 5)
	{
		// nasty switch statement is probably easier to grok than a bunch of
		// ifs marhshalling data into vars
		switch (counter)
		{
			case 0:
				id = atoi(token);
				break;
			case 1:
				strcpy(src, token);
				break;
			case 2:
				strcpy(dest, token);
				break;
			case 3:
				TTL = atoi(token);
				break;
			case 4:
				// handle a custom payload length
				payload = malloc(strlen(token) + 1);
				strcpy(payload, token);
				break;
		}

		counter++;
		token = strtok(NULL, selector);
	}

	// If we didn't parse the five expected packet segments
	if (counter < 4)
	{
		fprintf(stderr, "Malformed packet provided for %s", raw_packet);
		return -1;
	}

	int result = 0;
	*packet = Packet_new(id, src, dest, TTL, payload);
	if (packet->TTL <= 0)
	{
		result = -1;
	}

	if (counter == 5)
	{
		free(payload);
	}

	return result;

}

/**
 * Constructor for creating a new packet. Will automatically decrement TTL.
 */
Packet Packet_new(int id, char* src, char* dest, int TTL, char* payload)
{
	Packet packet;

	// allocate and initialize values
	packet.id = id;
	packet.src = malloc(strlen(src) + 1);
	strcpy(packet.src, src);
	packet.dest = malloc(strlen(dest) + 1);
	strcpy(packet.dest, dest);

	// automatically decrement TTL right off the bat
	packet.TTL = TTL - 1;
	packet.payload = malloc(strlen(payload) + 1);

	strcpy(packet.payload, payload);
	return packet;
}

/**
 * Marshal route segments into a route struct with format:
 *
 * 0 - <network‐address>
 * 1 - <net‐prefix‐length>
 * 2 - <nexthop>
 */
void add_new_router(RouterTable* table, char* address, int prefix_length, char* next_hop)
{
	// marshal data into Route
	Router* new_route = malloc(sizeof(Router));
	new_route->address = malloc(strlen(address) + 1);
	strcpy(new_route->address, address);
	new_route->prefix_length = prefix_length;
	new_route->next_hop = malloc(strlen(next_hop) + 1);
	strcpy(new_route->next_hop, next_hop);

	// Grow our RouteTable array if at max length
	if (table->size == table->max_size)
	{
		table->max_size *= 2;
		table->routes = realloc(table->routes, table->max_size * sizeof(Router*));
	}

	// add route to the array
	table->routes[table->size] = new_route;
	table->size = table->size + 1;
}

/**
 * Updates the statistics file based upon the state of the global statistics
 * struct.
 */
void output_statistics()
{
	// rewind pointer to the start of the file
	fprintf(
		stats_file,
		"expired packets: %d\nunroutable packets: %d\ndelivered direct: %d\nrouter B: %d\nrouter C: %d\n",
		stats.expired, stats.unroutable, stats.direct, stats.router_b, stats.router_c
	);

	rewind(stats_file);
	printf("Router stats updated.\n");
}

/**
 * Handles building the socket, binding, and listening it to the specified
 * port.
 */
int build_socket(int port)
{
	struct sockaddr_in sock;
	int socketfd;
	
	if ((socketfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
	{
		fprintf(stderr, "Unable to create a new socket.");
		exit(errno);
	}

	// 0 out
	memset((char *) &sock, 0, sizeof(sock));

	// Specify socket parameters
	sock.sin_family = AF_INET;
	sock.sin_port = htons(port);
	sock.sin_addr.s_addr = htonl(INADDR_ANY);

	// bind to specified socket
	if (bind(socketfd, (struct sockaddr*) &sock, sizeof(sock)) == -1)
	{
		fprintf(stderr, "Error binding socket on port %d\n", port);
		exit(errno);
	}

	return socketfd;
}

void signal_handler(int signal)
{
	output_statistics();
    printf("Terminating...");
	exit(0);
}

/**
 * Checks if an ip address is in the same subnet as the defined prefix.
 *
 * Returns 0 if same, -1 if not.
 */
int compare_subnet(int prefix_length, char* candidate, char* destination)
{
	// build our mask
	uint32_t net_mask = UINT_MAX;
	uint32_t shift_amnt = 32 - prefix_length;

	// shift right to unset lower bits, then shift back to get correct value
	net_mask = net_mask >> shift_amnt;
	net_mask = net_mask << shift_amnt;

	uint32_t destination_ip = parse_ipv4_string(destination);

	// gets the candidates subnet using the candidate with the mask
	uint32_t candidate_ip = parse_ipv4_string(candidate);
	uint32_t subnet = candidate_ip & net_mask;

	uint32_t masked_destination = destination_ip & net_mask;
	if (masked_destination == subnet)
	{
		return 0;
	}
	
	return -1;
}

/**
 * Adopted from:
 * http://stackoverflow.com/questions/10283703/conversion-of-ip-address-to-integer
 *
 * Which is a helper to convert IPv4 strings into unsigned ints for mask
 * comparison.
 */
uint32_t parse_ipv4_string(char* ipAddress)
{
	uint32_t ipbytes[4];
	sscanf(ipAddress, "%u.%u.%u.%u", &ipbytes[3], &ipbytes[2], &ipbytes[1], &ipbytes[0]);
	return ipbytes[0] | ipbytes[1] << 8 | ipbytes[2] << 16 | ipbytes[3] << 24;
}
