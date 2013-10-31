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

void print_sended(long sended)
{
	if(sended/(1024*1024))
		printf("\rsended: %ld Mb   ", sended/(1024*1024));
	else if(sended/1024)
		printf("\rsended: %ld kb   ", sended/1024);
	else
		printf("\rsended: %ld b", sended);
}

int main(int argc, char *argv[])
{
	int sockfd, client, port;
	struct sockaddr_in server;
	char buf[BUF_SIZE], command[5], filename[80];
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
	
	if(bind(sockfd, (struct sockaddr *)&server, sizeof(server)) < 0) 
	{
		perror("Binding error");
		exit(2);
	}
	
	printf("Server is working.\n");
	printf("Commands:\n");
	printf("\tsend <filepath+filename> - send file to client\n");
	printf("\texit - stop server and close this program\n");

	while(1) 
	{
		scanf("%s", &command);	// reading command
		
		if(!strcmp(command, "send"))
		{
			scanf("%s", &filename);
			file = fopen(filename, "r");
			if(file == NULL)
			{
				perror("Opening file error");
				continue;
			}

			listen(sockfd, 1);
			client = accept(sockfd, NULL, NULL);
			
			if(client < 0) 
			{ 
				perror("Accept error");
				continue;
			}
			
			send(client, basename(filename), sizeof(basename(filename)), 0);	// sending filename
			recv(client, buf, sizeof(buf), 0);
			int dpart = atoi(buf);
			printf("Download parts %d\n", dpart);
			
			long sended = 0, data;
			for(int i = 0; !feof(file); i++)
			{
				data = fread(buf, 1, sizeof(buf), file);
				sended += data;
				if(data != 0 && (dpart-1) < i)
					send(client, buf, data, 0);
				
				print_sended(sended);
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