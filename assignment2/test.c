#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "router.h"

/* Test Declarations */
void test_build_route_table();
void test_receive_packet();


/**
* Test runner.
 */
int main(int argc, char **argv)
{
    test_build_route_table();
    test_receive_packet();
}

void test_build_route_table()
{
    // basic parsing test
    char path[] = "./RT_A.txt";
    RouteTable* table = build_route_table(path);
    assert(table->size == 3);

    // make sure a route looks good
    Route* route = table->routes[1];
    assert(strcmp(route->address, "192.168.128.0") == 0);
    assert(strcmp(route->next_hop, "0") == 0);
    assert(route->prefix_length == 17);
}

void test_receive_packet()
{
    char raw_packet[] = "215, 192.168.192.4, 192.224.0.7, 64, \"Hello\"";
    Packet* packet = build_packet(raw_packet);

    // make sure all data got martialled in properly
    assert(packet->id == 215);
    assert(strcmp(packet->src, "192.168.192.4") == 0);
    assert(strcmp(packet->dest, "192.224.0.7") == 0);
    assert(packet->TTL == 63);
    assert(strcmp(packet->payload, "\"Hello\"") == 0);

    // make sure we drop anything with a TTL of 1, decremented to 0
    char raw_packet2[] = "215, 192.168.192.4, 192.224.0.7, 1, \"Hello\"";
    Packet* packet2 = build_packet(raw_packet2);
    assert(packet2 == NULL);
}
