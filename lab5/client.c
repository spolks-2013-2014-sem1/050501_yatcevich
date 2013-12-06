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

int recv_file_tcp(int server, struct sockaddr_in server_addr)
{
	long received, n, dpart = 0;
	char buf[BUF_SIZE], filename[MAX_FNAME_LEN], downloaded_parts[20];
	FILE *file;

	// some cleaning
	memset(buf, 0, BUF_SIZE);
	memset(filename, 0, MAX_FNAME_LEN);
	
	if(connect(server, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
	{
		perror("Connect error");
		exit(2);
	}

	if(recv(server, filename, MAX_FNAME_LEN, 0) < 0)	// recv filename
	{
		perror("Receiving filename error");
		close(server);
		exit(10);
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
		if(sockatmark(server) == 1)
		{
            recv(server, buf, 1, MSG_OOB);
            print_received(received);
        }

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
	return 0;
}

int recv_file_udp(int server, struct sockaddr_in server_addr)
{
	long received, n, dpart = 0;
	char buf[BUF_SIZE], filename[MAX_FNAME_LEN], downloaded_parts[20];
	FILE *file;

	// some cleaning
	memset(buf, 0, BUF_SIZE);
	memset(filename, 0, MAX_FNAME_LEN);
	
	if(connect(server, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
	{
		perror("Connect error");
		exit(2);
	}
printf("connected\n");
	while(recv(server, filename, MAX_FNAME_LEN, 0) < 0)	// recv filename
	{
		//perror("Receiving filename error");
		//close(server);
		//exit(10);
		sleep(300);
	}
printf("fname received\n");
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
	}
	printf("\n");
	fclose(file);
	return 0;
}

int start_client(int port, char *protocol)
{
	int server;
	struct sockaddr_in server_addr;

	// clearing server_addr struct			 
	memset(&server_addr, 0, sizeof(server_addr));
	
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port = htons(port);

	if(!strcmp(protocol, "tcp"))
	{
		server = socket(AF_INET, SOCK_STREAM, 0);
		
		if(server < 0)
		{
			perror("Opening socket error");
			exit(1);
		}
		
		recv_file_tcp(server, server_addr);
	}
	else if(!strcmp(protocol, "udp"))
	{
		server = socket(AF_INET, SOCK_DGRAM, 0);
		
		if(server < 0)
		{
			perror("Opening socket error");
			exit(1);
		}
		
		recv_file_udp(server, server_addr);
	}
	else
	{
		printf("Wrong protocol\n");
		exit(7);
	}
	
	close(server);
	return 0;
}

int main(int argc, char *argv[])
{
	int port;
	char protocol[5];

	if(argc > 1)
		printf("Program does not accept command line arguments.\n");
	
	printf("port: ");
	scanf("%d", &port);
	
	printf("protocol (tcp/udp): ");
	scanf("%s", protocol);
	
	start_client(port, protocol);

	return 0;
}