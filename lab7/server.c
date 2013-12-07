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
#include <pthread.h>

#include "../libspolks/libspolks.c"


#define BUF_SIZE 1024
#define MAX_FNAME_LEN 256
#define MAX_QUEUE 10

FILE *file;
char *filename;


void *send_file_tcp(void *socket);


int send_file_main(int socket)
{
	file = fopen(filename, "r");
	if(file == NULL)
	{
		perror("Opening file error");
		return 1;
	}

	printf("waiting for client...\n");
	listen(socket, MAX_QUEUE);
	
	while(1)
	{
		fseek(file, 0L, SEEK_SET);
		intptr_t client = accept(socket, NULL, NULL);
		if(client < 0) 
		{ 
			perror("Accept error");
			return 2;
		}
		pthread_t thrd;
		pthread_create(&thrd, NULL, send_file_tcp, (void*)client); 
	}
	fclose(file);
	return 0;
}

void *send_file_tcp(void *socket)
{
	pthread_detach(pthread_self());
	intptr_t error = -1;
	int client = (intptr_t) socket;
	long sent, dpart = 0, length, lets_send_0;
	char buf[BUF_SIZE];

	memset(buf, 0, BUF_SIZE);	// clearing buffer

	file = fopen(filename, "r");
	if(file == NULL)
		return (void*) error;

	// sending filename
	if(send(client, (char*)basename(filename), strlen((char*)basename(filename)) * sizeof(char), 0) < 0)
		return (void*) error;

	if(recv(client, buf, sizeof(buf), 0) < 0)
		return (void*) error;
	dpart = atol(buf);
	
	sent = dpart;
	fseek(file, dpart, SEEK_SET);	// skipping already sended part of file
	while(!feof(file))
	{
		length = fread(buf, 1, sizeof(buf), file);
		if(length != 0)
			if(send(client, buf, length, 0) < 0)
				return (void*) error;
		sent += length;
		
		if(sent - dpart > lets_send_0)	// sending special zero
		{
			//send(client, "0", 1, MSG_OOB);
			lets_send_0 += 1024 * 1024;
		}
		print_progress("total sent", sent);
	}
	printf("\nDone.\n");
	fclose(file);
	close(client);
	return NULL;
}

int send_file_udp(int socket)
{
	int client;
	long sent, dpart = 0, length, filesize, lets_send_0;
	struct sockaddr_in from;
    socklen_t fromlen = sizeof(from);
	char buf[BUF_SIZE];

	file = fopen(filename, "r");
	if(file == NULL)
	{
		perror("Opening file error");
		return 1;
	}

	recvfrom(socket, buf, 5, 0, (struct sockaddr*)&from, &fromlen); 	// receiving "hello"
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
	fclose(file);
	close(socket);
	return 0;
}

int main(int argc, char *argv[])
{
	int socket_tcp, socket_udp, client, port;
	struct sockaddr_in addr;
	char *command, *protocol;

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
				send_file_main(socket_tcp);
			else if(!strcmp(protocol, "udp"))
				send_file_udp(socket_udp);
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