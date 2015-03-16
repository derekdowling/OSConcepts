/**
 * A very simple packet router. 
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <strings.h>
#include "router.h"

void launch (int argc, char *argv[]) {

	if (argc != 4)
	{
		printf("Bad Args, should be <listening-port> <routing-table-path> <statistics-file-path>");
	}

	RouteTable* table;

	// parse out routes into Route array pointer from RT_A.txt
	table = build_route_table(argv[2]);

	for(int i = 0; i < table->size; i++)
	{
		free(table->routes[i]);
	}

	free(table);
}

/**
 * Parses out a route table struct from the provided table file path.
 */
RouteTable* build_route_table(char* table_path)
{
	char* token = NULL;
	RouteTable* table = RouteTable_new();
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
		add_new_route(table, address, prefix_length, next_hop);
	}

	return table;
}

/**
 * Allocates and initializes a new RouteTable.
 */
RouteTable* RouteTable_new()
{
	RouteTable* table = malloc(sizeof(RouteTable));
	table->size = 0;
	table->max_size = 20;
	table->routes = malloc(table->max_size * sizeof(Route*));
	return table;
}



/**
 * Handles parsing, and preparing an incoming packet for:
 * <packet ID>, <source IP>, <destination IP>, <TTL>, <payload>
 */
Packet* build_packet(char* raw_packet)
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

	Packet* packet = Packet_new(id, src, dest, TTL, payload);
	if (packet->TTL <= 0)
	{
		free(packet);
		return NULL;
	}

	free(payload);

	return packet;
}

/**
 * Constructor for creating a new packet. Will automatically decrement TTL.
 */
Packet* Packet_new(int id, char* src, char* dest, int TTL, char* payload)
{
	Packet* packet = malloc(sizeof(Packet));

	// allocate and initialize values
	packet->id = id;
	packet->src = malloc(strlen(src) + 1);
	strcpy(packet->src, src);
	packet->dest = malloc(strlen(dest) + 1);
	strcpy(packet->dest, dest);

	// automatically decrement TTL right off the bat
	packet->TTL = TTL - 1;
	packet->payload = malloc(strlen(payload) + 1);

	strcpy(packet->payload, payload);
	return packet;
}

/**
 * Marshal route segments into a route struct with format:
 *
 * 0 - <network‐address>
 * 1 - <net‐prefix‐length>
 * 2 - <nexthop>
 */
void add_new_route(RouteTable* table, char* address, int prefix_length, char* next_hop)
{
	// marshal data into Route
	Route* new_route = malloc(sizeof(Route));
	new_route->address = malloc(strlen(address) + 1);
	strcpy(new_route->address, address);
	new_route->prefix_length = prefix_length;
	new_route->next_hop = malloc(strlen(next_hop) + 1);
	strcpy(new_route->next_hop, next_hop);

	// Grow our RouteTable array if at max length
	if (table->size == table->max_size)
	{
		table->max_size *= 2;
		table->routes = realloc(table->routes, table->max_size * sizeof(Route*));
	}

	// add route to the array
	table->routes[table->size] = new_route;
	table->size = table->size + 1;
}
