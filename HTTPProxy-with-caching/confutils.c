/* methods to connect clients and server */

#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>

#include <stdlib.h>

#define MAXLENGTH 256


int startProxyServer() {
	int sd;
	char serverHost[MAXLENGTH];
	unsigned short serverPort;


    if((sd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("*--------- Error in socket() ---------* ");
        exit(1);
    }

    for(int a =0; a <2 ; a++) {
        
    }
    
    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = 0;
    memset(server.sin_zero, '\0', sizeof server.sin_zero);
    if(bind(sd, (struct sockaddr *)&server, sizeof(struct sockaddr_in)) < 0) {
        perror("*----------- Error in bind() -----------* ");
        exit(1);
    }


	if(listen(sd, 5) < 0) {
        perror("*---------- Error in listen() -----------* ");
        exit(1);
    }

    if(gethostname(serverHost, sizeof serverHost) < 0) {
        perror("*---------- Error in gethostname() ------------* ");
        exit(1);
    }
    struct hostent* h = gethostbyname(serverHost);

    socklen_t addr_size = sizeof(struct sockaddr_in);
    if(getsockname(sd, (struct sockaddr *) &server, &addr_size) == -1) {
        perror("*----------- Error in getsockname() -----------* ");
        exit(1);
    }
    serverPort = ntohs(server.sin_port);

	printf("*------------ admin: Proxy started on '%s' at '%hu' ----------*\n", h->h_name, serverPort);
	return (sd);
}

int hookToServer(char *serverHost, unsigned short serverPort) {
	int sd = 0;

    sd = socket(AF_INET, SOCK_STREAM, 0);

    struct hostent* lh = gethostbyname(serverHost);
    if(!lh) {
        perror("*----------- Error in getting hostname -----------* ");
        exit(1);
    }
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(serverPort);
    memcpy((void*)&addr.sin_addr, lh->h_addr_list[0], lh->h_length);
    memset(addr.sin_zero, '\0', sizeof addr.sin_zero);
    socklen_t addr_len = sizeof(addr);

    if(connect(sd, (struct sockaddr *)&addr, addr_len) == -1) {
        perror("*----------- Error in connecting to host -------------* ");
        exit(1);
    }

	
    if(getsockname(sd, (struct sockaddr *)&addr, &addr_len) == -1) {
        perror("*---------- Error in getting socket address ------------* " );
        exit(1);
    }
    	return (sd);
}

int readBytes(int sd, char *buffer, int n) {
	int toRead;
	char * ptr;

	toRead = n;
	ptr = buffer;
	while (toRead > 0) {
		int readbytes;

		readbytes = read(sd, ptr, toRead);
		if (readbytes <= 0) {
			if (readbytes == -1)
				perror("*--------- Error in read ----------*");
			return (0);
		}

		toRead -= readbytes;
		ptr += readbytes;
	}
	return (1);
}

