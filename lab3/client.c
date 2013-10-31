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

int main(int argc, char *argv[])
{
	int server, port, dpart;
	struct sockaddr_in server_addr;
	char buf[BUF_SIZE], filename[40];
	FILE *file;

	if(argc > 1)
		printf("Program does not accept command line arguments.\n");
	
	// clearing buffer and server_addr struct
	memset(buf, 0, BUF_SIZE);			 
	memset(&server_addr, 0, sizeof(server_addr));
	
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

	recv(server, filename, sizeof(filename), 0);
	dpart = 0;

	if(file = fopen(filename, "r+"))	// if file exists
	{
		while (!feof(file))
		{
			int b = fread(buf, 1, sizeof(buf), file);
			int size = ftell(file);
			printf("\rbytes read: %d, part: %d, pos: %d", b, dpart, size);
			dpart++;
		}
	}
	else
		file = fopen(filename, "w");

	if(file == NULL)
	{
	    perror("Opening file error");
	    exit(3);
	}
	
	char dparts[10];
	sprintf(dparts, "%i", dpart);
	send(server, dparts, strlen(dparts), 0);
	
	for(int i = 0; 1; i++) 
	{
		int nbytes = recv( server, buf, sizeof(buf), 0 );
		if(nbytes == 0)
		{
			printf("Done.\n");
			break;
		}
		if(nbytes < 0)
		{
			perror("Not get byte");
			break;
		}
		fwrite(buf, nbytes, 1, file);
		printf("\rBytes: %d, part: %d", nbytes, i);
	}
	printf("\n");
	fclose(file);
	close(server);
	return 0;
}