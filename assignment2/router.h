#ifndef ROUTER_H_
#define ROUTER_H_

typedef struct {
    char* address;
    char* next_hop;
    int prefix_length;
} Route;

typedef struct {
    Route** routes;
    int size;
    int max_size;
} RouteTable;

typedef struct {
    int id;
    char* src;
    char* dest;
    int TTL;
    char* payload;
} Packet;

/* Route Parsing */
RouteTable* RouteTable_new();
RouteTable* build_route_table(char* table_path);
void add_new_route(RouteTable* table, char address[16], int prefix_length, char next_hop[8]);

/* Packet Parsing */
Packet* build_packet();
Packet* Packet_new(int id, char* src, char* dest, int TTL, char* payload);

#endif
