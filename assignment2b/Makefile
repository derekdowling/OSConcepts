all:
	make file_server
	make file_client
file_server:
	gcc -std=c99 -m32 file_server.c -o file_server
test_file_server:
	rm file_server
	make file_server
	./file_server 8585 ./test_dir file_server_stats.txt
file_client:
	gcc -std=c99 -m32 file_client.c -o file_client
test_file_client:
	rm file_client
	make file_client
	./file_client 127.0.0.1 8585 picture.jpg
clean:
	rm file_client file_server server_logs.txt
package:
	tar -cvf dowling-asgn2b.tar file_server.c file_client.c Makefile
