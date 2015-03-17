#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

/* Test Declarations */
void test_build_router_table();
void test_receive_packet();
void test_find_destination_router();

RouterTable* table;

/**
* Test runner.
 */
int main(int argc, char **argv)
{
    test_build_router_table();
    test_receive_packet();
    test_find_destination_router();

    // now tear everything back down
	for(int i = 0; i < table->size; i++)
	{
		free(table->routes[i]);
	}
	free(table);
}

void test_build_router_table()
{
    // basic parsing test
    char path[] = "./RT_A.txt";
    table = build_router_table(path);
    assert(table->size == 3);

    // make sure a route looks good
    Router* router = table->routes[1];
    assert(strcmp(router->address, "192.168.128.0") == 0);
    assert(strcmp(router->next_hop, "0") == 0);
    assert(router->prefix_length == 17);

    free(router);
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

    free(packet);
    free(packet2);
}

void test_find_destination_router()
{
    Packet packet = (Packet) {125, "192.168.128.0", "192.168.192.0", 2, "\"yoooo\""};
    Router* router = find_destination_router(table, &packet);
    assert(router != NULL);
    assert(strcmp(router->next_hop, "RouterB") == 0);

    Packet packet2 = (Packet) {6290, "192.168.128.0", "192.168.567.0", 2, "\"Sweeet\""};
    Router* router2 = find_destination_router(table, &packet2);
    assert(router2 == NULL);
}
