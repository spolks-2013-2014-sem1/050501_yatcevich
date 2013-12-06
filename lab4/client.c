#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <signal.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <string.h>
#include <libgen.h>
#include <fcntl.h>
#include <errno.h>

#include "../libspolks/libspolks.c"


#define BUF_SIZE 1024
#define MAX_FNAME_LEN 256


long received = 0;
int server = 0;
void urg_handler(int signo)
{
	char small_buf;
	recv(server, &small_buf, 1, MSG_OOB);
	print_progress("received", received);
}

int main(int argc, char *argv[])
{
	int port;
	long n, dpart = 0;
	struct sockaddr_in server_addr;
	struct sigaction urg_signal;
	char buf[BUF_SIZE], filename[MAX_FNAME_LEN], downloaded_parts[20];
	FILE *file;

	if(argc > 1)
		printf("Program does not accept command line arguments.\n");
	
	// some cleaning
	memset(buf, 0, BUF_SIZE);			 
	memset(&server_addr, 0, sizeof(server_addr));
	memset(filename, 0, MAX_FNAME_LEN);
	memset(&urg_signal, 0, sizeof(server_addr));

	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	printf("port: ");
	scanf("%d", &port);
	server_addr.sin_port = htons(port);
	
	// opening socket
	server = socket(AF_INET, SOCK_STREAM, 0);
	if(server < 0)
	{
		perror("Opening socket error");
		exit(1);
	}

	if(connect(server, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
	{
		perror("Connect error");
		exit(2);
	}

	urg_signal.sa_handler = urg_handler;
	sigaction(SIGURG, &urg_signal, NULL);
	if(fcntl(server, F_SETOWN, getpid()) < 0) 
	{
		perror("fcntl error");
		exit(10);
	}

	if(recv(server, filename, MAX_FNAME_LEN, 0) < 0)	// recv filename
	{
		perror("Receiving filename error");
		close(server);
		exit(10);
	}

	if(file = fopen(filename, "r+"))	// if file exists
	{
		printf("Reading already received part of file %s...\n", filename);
		fseek(file, 0, SEEK_END);	// skipping already received part of file
		dpart = ftell(file);
		if(dpart)
			print_progress("read", dpart);
		printf("\n");
	}
	else
		file = fopen(filename, "w");

	if(file == NULL)
	{
	    perror("Opening file error");
	    close(server);
	    exit(3);
	}
	
	sprintf(downloaded_parts, "%li", dpart);
	send(server, downloaded_parts, strlen(downloaded_parts), 0);
	
	printf("Receiving file %s...\n", filename);
	received = 0;
	while(1) 
	{
		/*if(sockatmark(server))
		{
            
            print_progress("received", received);
        }*/

		n = recv(server, buf, sizeof(buf), 0);
		received += n;

		if(n == 0)
		{
			printf("\nDone.");
			break;
		}
		if(n < 0)
		{
			perror("Receiving error");
			break;
		}
		fwrite(buf, n, 1, file);
	}
	printf("\n");
	fclose(file);
	close(server);
	return 0;
}