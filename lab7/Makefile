CC=gcc

CFLAGS=-std=gnu99 -pthread


all: 
	$(CC) $(CFLAGS) server.c -o server
	$(CC) $(CFLAGS) client.c -o client

clean:
	rm -f server
	rm -f client