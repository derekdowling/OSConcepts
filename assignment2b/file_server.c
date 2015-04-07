/*
 * Copyright (c) 2008-2015 Bob Beck <beck@obtuse.com>, Derek Dowling
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <netdb.h>
#include <arpa/inet.h>

#include <time.h>
#include <err.h>
#include <errno.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define CHUNK_SIZE 1024
/* #define IP "127.0.0.1" */

void log_response(
	FILE* log_file,
	char* ip,
	unsigned short port,
	char* file_name,
	time_t start,
	char* result
);
static void kidhandler(int signum);

/**
 *
 */
int main(int argc,	char *argv[])
{
	struct sockaddr_in sockname, client;
	char buffer[CHUNK_SIZE];
	struct sigaction sa;
	socklen_t clientlen;
	int sd, port;
	pid_t pid;
	FILE* log_file;

	/* Check arg length */
	if (argc != 4)
	{
		fprintf(stderr, "usage: ./file_server <port> <file-dir> <log_path>\n");
		exit(-1);
	}

	/* assuming we've passed all checks, assign port */
	port = atoi(argv[1]);
	puts(argv[3]);

	/* Start deamonization by forking off the parent process and terminating
	 * if an error occurred. */
	pid = fork();
	if (pid < 0)
	{
		fprintf(stderr, "Failed to fork off parent while daemonizing.\n");
		exit(EXIT_FAILURE);
	}

	/* Success: Terminate Parent Process and continue on to post-daemoinzation */
	if (pid > 0)
	{
		exit(EXIT_SUCCESS);
	}

	/* Set new file permissions */
	umask(0);

	/* FROM HERE ON OUT, LOG TO FILE, OTHERWISE WE WONT SEE IT */
	log_file = fopen(argv[3], "w");
	if (log_file == NULL)
	{
		fprintf(stderr, "Unable to open or create log file: %s.\n", argv[3]);
		exit(errno);
	}
	/* set log buffer to 0 so we don't need to flush all over the place */
	setbuf(log_file, NULL);

	/* On success: The child process becomes session leader */
	if (setsid() < 0)
	{
		fprintf(log_file, "Error creating new session for process.\n");
		exit(EXIT_FAILURE);
	}

	/* now check the provided directory to serve */
	if(chdir(argv[2]) < 0)
	{
		fprintf(log_file, "Unable to find specified file director:%s.\n", argv[2]);
		exit(EXIT_FAILURE);
	}

	/* Close out the standard file descriptors */
	close(STDIN_FILENO);
	close(STDOUT_FILENO);
	close(STDERR_FILENO);

	/* prepare our socket info */
	memset(&sockname, 0, sizeof(sockname));
	sockname.sin_family = AF_INET;
	sockname.sin_port = htons(port);
	sockname.sin_addr.s_addr = htonl(INADDR_ANY);
	sd = socket(AF_INET, SOCK_STREAM, 0);

	if (sd == -1)
	{
		fprintf(log_file, "Failed setting up socket.\n");
		err(1, "Failed socket setup");
	}

	if (bind(sd, (struct sockaddr *) &sockname, sizeof(sockname)) == -1)
	{
		fprintf(log_file, "Failed binding socket to port: %u.\n", port);
		exit(errno);
	}

	if (listen(sd, 5) == -1)
	{
		fprintf(log_file, "Failed to listen via socket, errno: %d.\n", errno);
		exit(errno);
	}

	/*
	 * we're now bound, and listening for connections on "sd" -
	 * each call to "accept" will return us a descriptor talking to
	 * a connected client
	 *
	 * first, let's make sure we can have children without leaving
	 * zombies around when they die - we can do this by catching
	 * SIGCHLD.
	 */
	sa.sa_handler = kidhandler;
	sigemptyset(&sa.sa_mask);

	/*
	 * we want to allow system calls like accept to be restarted if they
	 * get interrupted by a SIGCHLD
	 */
	sa.sa_flags = SA_RESTART;
	if (sigaction(SIGCHLD, &sa, NULL) == -1)
	{
		fprintf(log_file, "Sigaction failed.\n");
		exit(errno);
	}

	/*
	 * finally - the main loop. accept connections and deal with 'em
	 */
	fprintf(log_file, "Server up and listening for connections on port %u\n", port);
	while(1)
	{
		int clientsd, client_pid;
		clientlen = sizeof(&client);

		clientsd = accept(sd, (struct sockaddr *)&client, &clientlen);
		if (clientsd == -1)
		{
			fprintf(log_file, "Connection accept failed.\n");
			exit(-1);
		}

		/*
		 * We fork child to deal with each connection, this way more
		 * than one client can connect to us and get served at any one
		 * time. There is a series of increasingly nesting if/else statements
		 * here to make teardown easier to deal with in the event something
		 * goes wrong.
		 */

		client_pid = fork();
		if(client_pid > 0)
		{
			/* log our start time */
			time_t start_time = time(NULL);

			ssize_t written, w;
			FILE* file;
			int retries;
			int result = 0;
			char file_name[CHUNK_SIZE], result_msg[32];

			/* This next little bit is for parsing out IP/Port for logging */
			struct sockaddr_in *sin = (struct sockaddr_in *) &client;
			char* ip = inet_ntoa(sin->sin_addr);
			unsigned short port = sin->sin_port;

			/* parse incoming request */
			if (read(clientsd, file_name, CHUNK_SIZE) > 0)
			{
				file = fopen(file_name, "rb");
				if (file != NULL)
				{
					/*
					 * stream the file to the client, being sure to
					 * handle a short write, or being interrupted by
					 * a signal before we could write anything.
					 */
					result = 1;
					written = 0;
					retries = 0;
					while (!feof(file))
					{
						int count = fread(buffer, 1, CHUNK_SIZE - 1, file);

						/* null terminate stream at wherever the reading finished */
						buffer[count] = 0;

						/* try streaming to socket */
						w = write(clientsd, buffer, count);
						
						if (w > 0)
						{
							written += count;
							retries = 0;
						}
						else if(w == -1 && retries < 2)
						{
							/* if something went wrong rewind file to start of last */
							/* chunk and try again */
							fseek(file, count * -1, SEEK_CUR);
							retries++;
						}
						else
						{
							result = -1;
							break;
						}
					}
					
					/*
					 * If we finished successfully, and we've sent more than a
					 * single chunk, terminate with "$" chunk.
					 */
					if (result == 1)
					{
						if (written > CHUNK_SIZE - 1)
						{
							if (write(clientsd, "$", 1) == -1)
							{
								fprintf(log_file, "Error sending EOF symbol.\n");
							}
						}

						time_t finish = time(NULL);
						log_response(
							log_file,
							ip,
							port,
							file_name,
							start_time,
							asctime(localtime(&finish))
						);

				
					} else {
						sprintf(result_msg, "transmission not completed");
						log_response(
							log_file,
							ip,
							port,
							file_name,
							start_time,
							"file not found"
						);
					}

					fclose(file);
				}
				else
				{
					log_response(
						log_file,
						ip,
						port,
						file_name,
						start_time,
						"file not found"
					);
				}
			}
			else
			{
				fprintf(log_file, "Error receiving incoming socket message.\n");
			}

			/* done serving request, terminate fork */
			shutdown(clientsd, SHUT_RDWR);
			close(clientsd);
			exit(0);
		}
		/* if we're dealing with the parent daemon, continue so we don't close anyhting */
		else if(client_pid == -1)
		{
			fprintf(log_file, "Connection fork failed.\n");
		}

	}

	fclose(log_file);
	exit(EXIT_SUCCESS);
}

/**
 * Outputs transfer information in a specific format
 */
void log_response(
	FILE* log_file,
	char* ip,
	unsigned short port,
	char* file_name,
	time_t start,
	char* result
) {
	fprintf(
		log_file,
		"%s %d %s %s %s\n",
		ip,
		ntohs(port),
		file_name,
		asctime(localtime(&start)),
		result
	);

	fflush(log_file);
}

/**
 * Sighandler for making sure no child processes turn into zombies.
 */
static void kidhandler(int signum)
{
	/* signal handler for SIGCHLD */
	waitpid(WAIT_ANY, NULL, WNOHANG);
}
