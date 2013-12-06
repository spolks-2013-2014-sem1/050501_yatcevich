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


int main(int argc, char *argv[])
{
	int sockfd, client, port;
	long sent, dpart, length, lets_send_0;
	struct sockaddr_in server;
	char buf[BUF_SIZE], *command, *filename;
	FILE *file;

	if(argc > 1)
		printf("Program does not accept command line arguments.\n");
	
	// clearing buffer and server struct
	memset(buf, 0, BUF_SIZE);			 
	memset(&server, 0, sizeof(server));
	
	// opening socket
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd < 0) 
	{
		perror("Opening socket error");
		exit(1);
	}
	
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	printf("port: ");
	scanf("%d", &port);
	server.sin_port = htons(port);
	
	if(bind(sockfd, (struct sockaddr*)&server, sizeof(server)) < 0) 
	{
		perror("Binding error");
		exit(2);
	}
	
	printf("Server is working.\n");
	printf("Commands:\n");
	printf("\tsend <filepath+filename> - send file to client\n");
	printf("\texit - stop server and close this program\n");

	command = (char*)calloc(5, sizeof(char));
	filename = (char*)calloc(MAX_FNAME_LEN, sizeof(char));

	while(1) 
	{
		scanf("%s", command);	// reading command
		
		if(!strcmp(command, "send"))
		{
			scanf("%s", filename);
			file = fopen(filename, "r");
			if(file == NULL)
			{
				perror("Opening file error");
				continue;
			}

			printf("waiting for a client...\n");
			listen(sockfd, 1);
			client = accept(sockfd, NULL, NULL);
			
			if(client < 0) 
			{ 
				perror("Accept error");
				continue;
			}
			
			// sending filename
			send(client, (char*)basename(filename), strlen((char*)basename(filename)) * sizeof(char), 0);
			
			recv(client, buf, sizeof(buf), 0);
			dpart = atol(buf);
			
			if(dpart)
				printf("%ld b already sent\n", dpart);

			sent = dpart;
			fseek(file, dpart, SEEK_SET);	// skipping already sent part of file
			while(!feof(file))
			{
				length = fread(buf, 1, sizeof(buf), file);
				if(length != 0)
					if(send(client, buf, length, 0) < 0)
					{
						perror("Sending error");
						exit(3);
					}
				sent += length;
				
				if(sent - dpart > lets_send_0)	// sending special zero
				{
					//sleep(1);
					send(client, "0", 1, MSG_OOB);
					lets_send_0 += 1024 * 1024;
					//sleep(1);
				}

				print_progress("total sent", sent);
			}
			printf("\nDone.\n");
			fclose(file);
			close(client);
		}
		else if(!strcmp(command, "exit"))
		{
			close(sockfd);
			exit(0);
		}
		else
			printf("bad command\n");
	}
	
	close(sockfd);
	return 0;
}