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
#include <time.h>

#define MAXBUF			1024

int build_socket(int port);

int main (int argc, char *argv[])
{
	int socketfd, port;
	struct sockaddr_in dest;
	char buffer[MAXBUF];
	FILE* save_file;

	if (argc != 4)
	{
		fprintf(
			stderr,
			"Invalid, should be: <server-ip> <port> <filename>"
		);
		exit(EXIT_FAILURE);
	}

	char* host = argv[1];
	port = atoi(argv[2]);
	char* file_name = argv[3];

	/* build our socket */
	/* socketfd = build_socket(port); */
	socketfd = socket(AF_INET, SOCK_STREAM, 0);

	/* define the target file_server address */
	dest.sin_family = AF_INET;
	dest.sin_port = htons(port);
	dest.sin_addr.s_addr = inet_addr(host);

	/* attempt to connect to the file server */
	if (connect(socketfd, (struct sockaddr*) &dest, sizeof(dest)) == -1)
	{
		fprintf(
			stderr,
			"Failed to connect to host %s on port %u.\n",
			host,
			port
		);
		exit(errno);
	}

	if (write(
			socketfd,
			file_name,
			strlen(file_name) + 1) == -1
	) {
		fprintf(stderr, "Error sending file request to client.\n");
		exit(EXIT_FAILURE);
	}

	save_file = fopen(file_name, "wb");
	if (save_file == NULL)
	{
		fprintf(stderr, "Error creating save file,\n");
		exit(EXIT_FAILURE);
	}

	// TODO: finish setting up 
	time_t last_chunk = time(NULL);
	for(int res ;;)
	{
		res = recv(socketfd, buffer, sizeof(buffer), MSG_DONTWAIT);
		if (res == -1)
		{
			if (errno == EAGAIN || errno == EWOULDBLOCK)
			{
				if (time(NULL) > last_chunk + 5)
				{
					fprintf(stderr, "File server timed out.\n");
					break;
				}

				continue;
			}
			else
			{
				fprintf(stderr, "Connection refused. Errno: %d\n", errno);
				break;
			}
		}
		if(res == 0)
		{
			break;
		}
		else if(res > 0)
		{
			/* if we receive exactly '$\0' it's EOF! */
			if (res == 2 && buffer[0] == '$')
			{
				break;
			}

			if (fwrite(buffer, sizeof(char), res, save_file) < res)
			{
				fprintf(stderr, "Error streaming save buffer to file. Errno: %d\n", errno);
				break;
			}

			last_chunk = time(NULL);
		}
	}

	printf("%s downloaded successfully!\n", file_name);

	// now tear everything back down
	close(socketfd);
	fclose(save_file);
	exit(EXIT_SUCCESS);
}
