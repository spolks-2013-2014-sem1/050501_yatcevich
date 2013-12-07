#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>
#include <netdb.h>
#include <stdio.h>
#include <libgen.h>

#include "../libspolks/libspolks.c"


#define BUF_SIZE 1024
#define MAX_FNAME_LEN 256


int send_file_tcp(char *filename, int socket)
{
	int client;
	long sent, dpart = 0, length, lets_send_0;
	pid_t pid;
	char buf[BUF_SIZE];
	FILE *file;

	memset(buf, 0, BUF_SIZE);	// clearing buffer

	file = fopen(filename, "r");
	if(file == NULL)
	{
		perror("Opening file error");
		return 1;
	}

	while(1)
	{
		printf("waiting for client...\n");
		listen(socket, 1);
		client = accept(socket, NULL, NULL);
		
		if(client < 0) 
		{ 
			perror("Accept error");
			return 2;
		}
		
		fseek(file, 0L, SEEK_SET);
		pid = fork();
		if(pid < 0)
		{
			perror("Creating process error");
			return 3;
		}
		if(pid > 0)		// parent process
			continue;

		// child process here
		// sending filename
		send(client, (char*)basename(filename), strlen((char*)basename(filename)) * sizeof(char), 0);

		recv(client, buf, sizeof(buf), 0);
		dpart = atol(buf);
		
		sent = dpart;
		fseek(file, dpart, SEEK_SET);	// skipping already sended part of file
		while(!feof(file))
		{
			length = fread(buf, 1, sizeof(buf), file);
			if(length != 0)
				send(client, buf, length, 0);
			sent += length;
			
			if(sent - dpart > lets_send_0)	// sending special zero
			{
				send(client, "0", 1, MSG_OOB);
				lets_send_0 += 1024 * 1024;
				print_progress2("total sent", sent, client);
			}
		}
		printf("\nDone (client %d)\n", client);
		close(client);
		exit(0);
	}
	fclose(file);
	return 0;
}

int send_file_udp(char *filename, int socket)
{
	int client;
	long sent, dpart = 0, length, filesize, lets_send_0;
	struct sockaddr_in from;
    socklen_t fromlen = sizeof(from);
	pid_t pid;
	char buf[BUF_SIZE];
	FILE *file;

	file = fopen(filename, "r");
	if(file == NULL)
	{
		perror("Opening file error");
		return 1;
	}

	while(1)
	{
		recvfrom(socket, buf, 5, 0, (struct sockaddr*)&from, &fromlen); 	// receiving "hello"
		
		pid = fork();
		if(pid < 0)
		{
			perror("Creating process error");
			return 3;
		}
		if(pid > 0)
			continue;	// parent process

		// child process
		memset(buf, 0, BUF_SIZE);	// clearing buffer

		// sending filename
		sendto(socket, (char*)basename(filename), strlen((char*)basename(filename)) * sizeof(char), 0,
			(struct sockaddr*)&from, fromlen);
		
		// sending filesize
		fseek(file, dpart, SEEK_END);
		filesize = ftell(file);
		sendto(socket, &filesize, sizeof(long), 0, (struct sockaddr*)&from, fromlen);

		// receiving downloaded part size
		recvfrom(socket, buf, sizeof(buf), 0, (struct sockaddr*)&from, &fromlen);
		dpart = atol(buf);
		
		sent = dpart;
		fseek(file, dpart, SEEK_SET);	// skipping already sended part of file
		while(!feof(file))
		{
			length = fread(buf, 1, sizeof(buf), file);
			
			if(length)
				length = sendto(socket, buf, length, 0, (struct sockaddr*)&from, fromlen);
			if(length < 0)
			{
				perror("Packet lost error");
				return -1;
			}
			sent += length;
			print_progress("total sent", sent);
		}
		dpart = -1;
		sendto(socket, &dpart, 1, 0, (struct sockaddr*)&from, fromlen);
		printf("\nDone.\n");
	}
	fclose(file);
	close(socket);
	return 0;
}

int main(int argc, char *argv[])
{
	int socket_tcp, socket_udp, client, port;
	struct sockaddr_in addr;
	char *command, *filename, *protocol;

	if(argc > 1)
		printf("Program does not accept command line arguments.\n");
				 
	memset(&addr, 0, sizeof(addr));	// clearing addr struct
	
	// opening sockets
	socket_tcp = socket(AF_INET, SOCK_STREAM, 0);
	socket_udp = socket(AF_INET, SOCK_DGRAM, 0);
	if(socket_tcp < 0 || socket_udp < 0) 
	{
		perror("Opening socket error");
		exit(1);
	}
	
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	printf("port: ");
	scanf("%d", &port);
	addr.sin_port = htons(port);
	
	if(bind(socket_tcp, (struct sockaddr*)&addr, sizeof(addr)) < 0) 
	{
		perror("Binding error");
		exit(2);
	}
	
	if(bind(socket_udp, (struct sockaddr *)&addr, sizeof(addr)) < 0)
	{
		perror("Connect udp socket error");
		exit(3);
	}

	printf("Server is working.\n");
	printf("Commands:\n");
	printf("\tsend <udp/tcp> <filepath+filename> - send file to client\n");
	printf("\texit - stop server\n");

	command = (char*)calloc(5, sizeof(char));
	protocol = (char*)calloc(5, sizeof(char));
	filename = (char*)calloc(MAX_FNAME_LEN, sizeof(char));

	while(1) 
	{
		scanf("%s", command);	// reading command
		
		if(!strcmp(command, "send"))
		{
			scanf("%s", protocol);
			scanf("%s", filename);

			if(!strcmp(protocol, "tcp"))
				send_file_tcp(filename, socket_tcp);
			else if(!strcmp(protocol, "udp"))
				send_file_udp(filename, socket_udp);
			else
				printf("Protocol is not specified. Valid values are 'tcp' and 'udp'.\n");	
		}
		else if(!strcmp(command, "exit"))
		{
			close(socket_tcp);
			close(socket_udp);
			exit(0);
		}
		else
			printf("bad command\n");
	}
	
	close(socket_tcp);
	close(socket_udp);
	return 0;
}