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
	socketfd = build_socket(port);

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

	if (send(
			socketfd,
			file_name,
			strlen(buffer) + 1,
			0) == -1
	) {
		fprintf(stderr, "Error sending file request to client.\n");
		exit(EXIT_FAILURE);
	}

	save_file = fopen(file_name, "w");
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
				fprintf(stderr, "Error when reading incoming file stream. %d\n", errno);
				break;
			}
		}
		if(res == 0)
		{
			break;
		}
		else
		{
			if (fwrite(buffer, sizeof(char), sizeof(res), save_file) < res)
			{
				fprintf(stderr, "Error streaming save buffer to file.\n");
				break;
			}

			last_chunk = time(NULL);
		}
	}

	// now tear everything back down
	close(socketfd);
	fclose(save_file);
	exit(EXIT_SUCCESS);
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
