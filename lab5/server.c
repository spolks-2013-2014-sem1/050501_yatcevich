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

void print_sended(long sended)
{
	if(sended/(1024*1024))
		printf("\rsended: %ld Mb   ", sended/(1024*1024));
	else if(sended/1024)
		printf("\rsended: %ld kb   ", sended/1024);
	else
		printf("\rsended: %ld b", sended);
}

int send_file_tcp(char *filename, int socket)
{
	int client;
	long sended, dpart = 0, length, lets_send_0;
	char buf[BUF_SIZE];
	FILE *file;

	memset(buf, 0, BUF_SIZE);	// clearing buffer

	file = fopen(filename, "r");
	if(file == NULL)
	{
		perror("Opening file error");
		return 1;
	}

	listen(socket, 1);
	client = accept(socket, NULL, NULL);
	
	if(client < 0) 
	{ 
		perror("Accept error");
		return 2;
	}
	
	// sending filename
	send(client, (char*)basename(filename), strlen((char*)basename(filename)) * sizeof(char), 0);

	recv(client, buf, sizeof(buf), 0);
	dpart = atol(buf);
	
	sended = dpart;
	fseek(file, dpart, SEEK_SET);	// skipping already sended part of file
	while(!feof(file))
	{
		length = fread(buf, 1, sizeof(buf), file);
		if(length != 0)
			send(client, buf, length, 0);
		sended += length;
		
		if(sended - dpart > lets_send_0)	// sending special zero
		{
			send(client, "0", 1, MSG_OOB);
			lets_send_0 += 1024 * 1024;
			print_sended(sended);
		}
	}
	printf("\nDone.\n");
	fclose(file);
	close(client);
	return 0;
}

int send_file_udp(char *filename, int socket)
{
	int client;
	long sended, dpart = 0, length, lets_send_0;
	char buf[BUF_SIZE];
	FILE *file;

	memset(buf, 0, BUF_SIZE);	// clearing buffer

	file = fopen(filename, "r");
	if(file == NULL)
	{
		perror("Opening file error");
		return 1;
	}

	//listen(socket, 1);
	//client = accept(socket, NULL, NULL);
	
	// sending filename
	while(send(socket, (char*)basename(filename), strlen((char*)basename(filename)) * sizeof(char), 0) < 0)
		sleep(300);
		
printf("fname sended\n");	

	recv(socket, buf, sizeof(buf), 0);
	dpart = atol(buf);
	
	sended = dpart;
	fseek(file, dpart, SEEK_SET);	// skipping already sended part of file
	while(!feof(file))
	{
		length = fread(buf, 1, sizeof(buf), file);
		if(length != 0)
			send(socket, buf, length, 0);
		sended += length;
		
		print_sended(sended);
	}
	printf("\nDone.\n");
	fclose(file);
	close(socket);
	return 0;
}

int main(int argc, char *argv[])
{
	int socket_tcp, socket_udp, client, port;
	struct sockaddr_in server;
	char *command, *filename, *protocol;

	if(argc > 1)
		printf("Program does not accept command line arguments.\n");
				 
	memset(&server, 0, sizeof(server));	// clearing server struct
	
	// opening sockets
	socket_tcp = socket(AF_INET, SOCK_STREAM, 0);
	socket_udp = socket(AF_INET, SOCK_DGRAM, 0);
	if(socket_tcp < 0 || socket_udp < 0) 
	{
		perror("Opening socket error");
		exit(1);
	}
	
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	printf("port: ");
	scanf("%d", &port);
	server.sin_port = htons(port);
	
	if(bind(socket_tcp, (struct sockaddr*)&server, sizeof(server)) < 0) 
	{
		perror("Binding error");
		exit(2);
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