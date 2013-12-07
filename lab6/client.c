#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/wait.h>
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

int recv_file_tcp(struct sockaddr_in server_addr)
{
	long n, dpart = 0;
	struct sigaction urg_signal;
	char buf[BUF_SIZE], filename[MAX_FNAME_LEN], downloaded_parts[20];
	FILE *file;

	// some cleaning
	memset(buf, 0, BUF_SIZE);
	memset(filename, 0, MAX_FNAME_LEN);
	memset(&urg_signal, 0, sizeof(server_addr));

	if(connect(server, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
	{
		perror("Connect error");
		exit(2);
	}

	urg_signal.sa_handler = urg_handler;
	sigaction(SIGURG, &urg_signal, NULL);
	
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
	
	printf("Receiving file %s...\n", filename);
	received = 0;
	while(1) 
	{
		if(sockatmark(server))
		{
			char small_buf;
			recv(server, &small_buf, 1, MSG_OOB);
			print_progress("received", received);
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

int recv_file_udp(struct sockaddr_in addr)
{
	long n, dpart = 0, filesize;
	socklen_t addrlen = sizeof(addr);
	char buf[BUF_SIZE], filename[MAX_FNAME_LEN], downloaded_parts[20];
	FILE *file;

	// some cleaning
	memset(buf, 0, BUF_SIZE);
	memset(filename, 0, MAX_FNAME_LEN);
	
	sendto(server, "hello", 5, 0, (struct sockaddr*)&addr, addrlen);	// send "hello"

	// recv filename
	if(recvfrom(server, filename, MAX_FNAME_LEN, 0, 
		(struct sockaddr *)&addr, &addrlen) < 0)
	{
		perror("Receiving filename error");
		close(server);
		exit(10);
	}

	// recv filesize
	recvfrom(server, &filesize, sizeof(long), 0, (struct sockaddr *)&addr, &addrlen);

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
	sendto(server, downloaded_parts, strlen(downloaded_parts), 0, (struct sockaddr*)&addr, addrlen);
	
	printf("Receiving file %s...\n", filename);
	received = 0;
	while(1) 
	{
		n = recvfrom(server, buf, sizeof(buf), 0, (struct sockaddr*)&addr, &addrlen);
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
		if(n == 1 && buf[0] == -1)
		{
			printf("\nDone.");
			break;
		}
		fwrite(buf, n, 1, file);
		print_progress("received", received);
	}
	printf("\n");
	if(received < filesize)
		printf("Warning: some packets were lost\n");
	fclose(file);
	return 0;
}

int start_client(int port, char *protocol)
{
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
		
		recv_file_tcp(server_addr);
	}
	else if(!strcmp(protocol, "udp"))
	{
		server = socket(AF_INET, SOCK_DGRAM, 0);
		
		if(server < 0)
		{
			perror("Opening socket error");
			exit(1);
		}
		
		recv_file_udp(server_addr);
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