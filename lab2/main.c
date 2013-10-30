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
	int sockfd, client, port;
	struct sockaddr_in server;
	char buf[BUF_SIZE];
	
	if(argc > 1)
		printf("Program does not accept command line arguments.\n");
	
	// clearing buffer and server struct
	memset(buf, 0, BUF_SIZE);			 
	memset(&server, 0, sizeof(server));
	
	// opening socket
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	
	if (sockfd < 0) {
		perror("Opening socket error");
		exit(1);
	}
	
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	printf("port: ");
	scanf("%d", &port);
	server.sin_port = htons(port);
	
	if (bind(sockfd, (struct sockaddr *)&server, sizeof(server)) < 0) {
		perror("Binding error");
		exit(2);
	}
	
	printf("server is working\n");
	
	while(1) {
		listen(sockfd, 1);	// request queue size = 1
		client = accept(sockfd, NULL, NULL);
				
		if (client < 0) { 
			perror("Accept error");
			exit(3);
		}
		
		// receiving and sending back client's messages
		int data;
		while(data = recv(client, buf, BUF_SIZE, 0))
			send(client, buf, data, 0);
		
		close(client);
	}
	
	close(sockfd);
	return 0;
}