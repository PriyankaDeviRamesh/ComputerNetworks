#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <pthread.h>

#include "cache.h"
#include "confutils.h"

pthread_mutex_t cache_mut = PTHREAD_MUTEX_INITIALIZER;
cache proxy_cache = {64, 0, NULL};
void* process(void* ptr);

typedef struct
{
    int sock;
    struct sockaddr_in sin_cli;
    socklen_t len;
} sockConn;

char* getURL(char* msg) {
    char* start = strstr(msg, "GET") + 4;
    char* end = strchr(start, ' ');
    char* url = (char*) malloc(end-start);
    strncpy(url, start, end-start);
    return url;
}

void getPort(char* msg, char** host, char** port) {
    char* start = strstr(msg, "Host:") + 6;
    char* end = strchr(start, '\n');
    char* colon = strchr(start, ':');
    if(colon < end) {
        *host = (char*)malloc(colon-start+1);
        strncpy(*host, start, colon-start);
        (*host)[colon-start] = '\0';
        *port = (char*)malloc(end-colon);
        strncpy(*port, colon+1, end-colon-1);
        (*port)[end-colon] = '\0';
    }
    else {
        *host = (char*)malloc(end-start);
        strncpy(*host, start, end-start);
        (*host)[end-start-1] = '\0';
        (*port) = (char*)malloc(3);
        (*port)[0] = '8'; (*port)[1] = '0'; (*port)[2] = '\0';
    }
    
}

size_t getContentLen(char* msg) {
    char* start = strstr(msg, "Content-Length: ") + strlen("Content-Length: ");
    char* end = strchr(start, '\r');
    char* charlen = (char*)malloc(end-start+1);
    strncpy(charlen, start, end-start);
    charlen[end-start] = '\0';
    
    size_t l = atoi(charlen);
    free(charlen);
    return l;
}

