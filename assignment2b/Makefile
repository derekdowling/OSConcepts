all:
	make file_server
	make file_client
file_server:
	gcc -std=c99 -m32 file_server.c -o file_server
test_file_server:
	make file_server
	./file_server 8585  router_stats.txt
file_client:
	gcc -std=c99 -m32 file_client.c -o pktgen
test_file_client:
	make file_client
	./file_client 192.168.10.1 8585 picture.jpg
clean:
	rm file_client file_server server_logs.txt
package:
	tar -cvf dowling-asgn2b.tar file_server.c file_client.c Makefile
