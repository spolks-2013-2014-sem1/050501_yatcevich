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
#include <fcntl.h>

#include "../libspolks/libspolks.c"


#define BUF_SIZE 1024
#define MAX_FNAME_LEN 256

fd_set readset, writeset;


int send_file_tcp(char *filename, int socket)
{
	int client;
	long sent, dpart = 0, length, lets_send_0, max_sockets_num = getdtablesize();
	long *file_positions = (long*)calloc(max_sockets_num, sizeof(long));
	short *fnames_to_send = (short*)calloc(max_sockets_num, sizeof(short));
	struct sockaddr_in from;  
	socklen_t fromlen = sizeof(struct sockaddr_in);
	char buf[BUF_SIZE];
	FILE *file;

	listen(socket, max_sockets_num);

	// clearing buffer and FDs
	memset(buf, 0, BUF_SIZE);
	FD_ZERO(&readset);
	FD_ZERO(&writeset);

	file = fopen(filename, "r");
	if(file == NULL)
	{
		perror("Opening file error");
		return 1;
	}

	while(1)
	{
		FD_SET(socket, &readset);
		FD_SET(socket, &writeset);
		select(max_sockets_num, &readset, &writeset, NULL, NULL);
		
		if(FD_ISSET(socket, &readset))
		{
			fromlen = sizeof(from);
			client = accept(socket, (struct sockaddr *) &from, &fromlen);
			fcntl(client, F_SETFL, O_NONBLOCK);
			FD_SET(client, &writeset);
			file_positions[client] = 0;
			fnames_to_send[client] = 1;
			continue;
		}
		
		for(client = 0; client < max_sockets_num; client++)
		{
			if ((client != socket) && FD_ISSET(client, &writeset))
			{
				if(fnames_to_send[client])
				{
					if(send(client, (char*)basename(filename), 
						strlen((char*)basename(filename)) * sizeof(char), 0) < 0)
					{
						perror("Sending filename error");
						close(client);
						FD_CLR(client, &writeset);
						continue;
					}
					fnames_to_send[client] = 0;
					continue;
				}

				if(file_positions[client] % (1024 * 1024) < 100)	// sending special zero
				{
					//send(client, "0", 1, MSG_OOB);
				}

				fseek(file, file_positions[client], SEEK_SET);	// skipping already sended part of file
				length = fread(buf, 1, sizeof(buf), file);
				if(length == 0)
				{
					printf("Sending completed (client %d).\n", client);
					close(client);
					FD_CLR(client, &writeset);
					continue;
				}
				if(send(client, buf, length, 0) < 0)
				{
					perror("Sending error");
					close(client);
					FD_CLR(client, &writeset);
					continue;
				}
				file_positions[client] += length;
				print_progress2("sent", file_positions[client], client);
			}
		}
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
	char buf[BUF_SIZE];
	FILE *file;

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