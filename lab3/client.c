#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>
#include <netdb.h>
#include <stdio.h>

#define BUF_SIZE 1024
#define MAX_FNAME_LEN 256

void print_received(long received)
{
	if(received/(1024*1024))
		printf("\rreceived: %ld Mb   ", received/(1024*1024));
	else if(received/1024)
		printf("\rreceived: %ld kb   ", received/1024);
	else
		printf("\rreceived: %ld b", received);
}

int main(int argc, char *argv[])
{
	int server, port, filename_length;
	long received, n, dpart = 0;
	struct sockaddr_in server_addr;
	char buf[BUF_SIZE], filename[MAX_FNAME_LEN], downloaded_parts[20];
	FILE *file;

	if(argc > 1)
		printf("Program does not accept command line arguments.\n");
	
	// some cleaning
	memset(buf, 0, BUF_SIZE);			 
	memset(&server_addr, 0, sizeof(server_addr));
	memset(filename, 0, MAX_FNAME_LEN);
	
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
		close(server);
		exit(2);
	}
	
	if(recv(server, filename, MAX_FNAME_LEN, 0) < 0)	// recv filename
	{
		perror("Receiving filename error");
		close(server);
		exit(11);
	}

	if(file = fopen(filename, "r+"))	// if file exists
	{
		for(dpart = 0; !feof(file); dpart += fread(buf, 1, sizeof(buf), file))
		{
			if(!dpart)
				printf("Reading already received part of file %s...\n", filename);
			print_received(dpart);
		}
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
		print_received(received);
	}
	printf("\n");
	fclose(file);
	close(server);
	return 0;
}