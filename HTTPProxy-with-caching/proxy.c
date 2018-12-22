/* Proxy server */

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
#include "proxy.h"

#define BLOCKSIZE 4096

int main(int argc, char* argv[])
{
    int serverSock;
    pthread_t thread;
    sockConn * conn;

    proxy_cache.tbl = (cacheElem*) malloc(64 * sizeof(cacheElem));


    if (argc != 1) {
        fprintf(stderr, "*---------  Correct Usage: %s ---------* \n", argv[0]);
        exit(1);
    }

    serverSock = startProxyServer();
    if (serverSock == -1) {
        perror("*---------- Failed to start Proxy server -----------* ");
        exit(1);
    }

    while (1) {
        conn = (sockConn *)malloc(sizeof(sockConn));
        conn->sock = accept(serverSock, (struct sockaddr*)&conn->sin_cli, &conn->len);
        if (conn->sock<0) {
            free(conn);
            continue;
        }
        pthread_create(&thread,0, process, (void *)conn);
        pthread_detach(thread);
    }
}


unsigned long long time_elapsed(struct timeval time1, struct timeval time2) {
    return 1000*(time2.tv_sec - time1.tv_sec) + (time2.tv_usec - time1.tv_usec)/1000;
}

void* process(void* ptr)
{
    if (!ptr) pthread_exit(0);
    struct timeval time1, time2;
    gettimeofday(&time1, NULL);

    sockConn * conn = (sockConn*)ptr;
    int client_sd = conn->sock;

    struct hostent *hp;
 
    if( getpeername(client_sd, (struct sockaddr *)&conn->sin_cli, &conn->len) < 0 ){
        perror(" *------------ Unable to retrieve peername -------------* ");
        exit(1);
    }

    hp = gethostbyaddr(&conn->sin_cli.sin_addr, sizeof(conn->sin_cli.sin_addr), AF_INET);
    if(!hp){
        perror(" *---------- Unable to retrieve hostname ---------* ");
        exit(1);
    }


    size_t msg_sz = BLOCKSIZE;
    size_t msgLen = 0;
    char buff[BLOCKSIZE]; buff[0] = '\0';
    char* msg = (char*) malloc(msg_sz);
    if(!msg) {
        fprintf(stderr, " *--------- Error in allocating memory ----------* \n");
        exit(4);
    }

    while (strstr(msg, "\r\n\r\n") == NULL) {
        ssize_t recvSize = recv(client_sd, buff, BLOCKSIZE, 0);
        if(recvSize == 0) break;
        else if(recvSize < 0) {
            fprintf(stderr, "*----------- Error in receiving message ----------* \n");
            exit(5);
        }
        else {
            msgLen += recvSize;
            if(msgLen > msg_sz) {
                msg_sz *= 2;
                msg = (char*) realloc(msg, msg_sz);
                if(!msg) {
                    fprintf(stderr, "*-------- Error in allocating memory ----------* \n");
                    exit(4);
                }
            }
            msg[msgLen] = '\0';
        }
        strcat(msg, buff);
    }

    if(strstr(msg, "GET") == NULL) {
        fprintf(stderr, "*-------- HTTP requests has to GET requests ---------* \n");
        exit(5);
    }
    else msg = strstr(msg, "GET");

    char* url = getURL(msg);
    cacheElem* Elem = fetchCache(&cache_mut, &proxy_cache, url);

  
    if(!Elem) {
        char* destHost = NULL;
        char* destPort = NULL;
        getPort(msg, &destHost, &destPort);

        int dest_sd = hookToServer(destHost, (unsigned short) atoi(destPort));
        write(dest_sd, msg, strlen(msg));

        char* tmp = (char*) malloc(BLOCKSIZE);
        ssize_t recvSize = recv(dest_sd, buff, 8192, 0);

        write(client_sd, buff, recvSize);

        size_t content_length = getContentLen(buff);
        msgLen = recvSize;
        msg_sz = BLOCKSIZE;


        while(msgLen < content_length) {
            recvSize = recv(dest_sd, buff, BLOCKSIZE, 0);
            if(recvSize < 0) {
                fprintf(stderr, "*-------- Error in receiving message ----------* \n");
                exit(5);
            }
            else if(recvSize + msgLen > msg_sz) {
                msg_sz *= 2;
                tmp = (char*) realloc(tmp, msg_sz);
            }
            strncat(tmp, buff, recvSize);
            write(client_sd, buff, recvSize);
            memset(buff, 0, sizeof buff);
            msgLen += recvSize;
        }


        addCache(&cache_mut, &proxy_cache, url, tmp, content_length, msgLen);
        free(destHost);
        free(destPort);

        gettimeofday(&time2, NULL);
        printf("%s|%s|CACHE_MISS|%lu|%llu\n",
                inet_ntoa(conn->sin_cli.sin_addr), url, content_length, time_elapsed(time1,time2));
    }
    else {
        gettimeofday(&time2, NULL);
        write(client_sd, Elem->content, Elem->totalSize);
        printf("%s|%s|CACHE_HIT|%lu|%llu\n",
                inet_ntoa(conn->sin_cli.sin_addr), url, Elem->bodySize, time_elapsed(time1,time2));
        free(url);
    }

    close(client_sd);
    free(conn);

    return NULL;
}
