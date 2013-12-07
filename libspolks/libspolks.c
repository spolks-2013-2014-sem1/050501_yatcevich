#include <stdio.h>

void print_progress(char *str, long value)
{
	if(value/(1024*1024))
		printf("\r%s: %ld Mb   ", str, value/(1024*1024));
	else if(value/1024)
		printf("\r%s: %ld kb   ", str, value/1024);
	else
		printf("\r%s: %ld b", str, value);
}


void print_progress2(char *str, long value, int client)
{
	if(value/(1024*1024))
		printf("\r%s (client %d): %ld Mb   ", str, client, value/(1024*1024));
	else if(value/1024)
		printf("\r%s (client %d): %ld kb   ", str, client, value/1024);
	else
		printf("\r%s (client %d): %ld b", str, client, value);
}