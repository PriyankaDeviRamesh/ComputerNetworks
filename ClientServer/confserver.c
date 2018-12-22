/*--------------------------------------------------------------------*/
/* conference server */

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h> 
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <arpa/inet.h> 
#include <netdb.h>
#include <time.h> 
#include <errno.h>

#include <stdlib.h>

extern char * recvtext(int sd);
extern int sendtext(int sd, char *msg);

extern int startserver();
/*--------------------------------------------------------------------*/

/*--------------------------------------------------------------------*/
int fd_isset(int fd, fd_set *fsp) {
    return FD_ISSET(fd, fsp);
}
/* main routine */
int main(int argc, char *argv[]) {
    int servsock; /* server socket descriptor */

    fd_set livesdset; /* set of live client sockets */
    int livesdmax; /* largest live client socket descriptor */

    /* check usage */
    if (argc != 1) {
        fprintf(stderr, "usage : %s\n", argv[0]);
        exit(1);
    }

    /* get ready to receive requests */
    servsock = startserver();
    if (servsock == -1) {
        perror("Error on starting server: ");
        exit(1);
    }

    /*
     FILL HERE:
     init the set of live clients
     */


    /* receive requests and process them */
    while (1) {
        int frsock; /* loop variable */

        /*
         FILL HERE
         wait using select() for
         messages from existing clients and
         connect requests from new clients
         */


        /* look for messages from live clients */
        for (frsock = 3; frsock <= livesdmax; frsock++) {
            /* skip the listen socket */
            /* this case is covered separately */
            if (frsock == servsock)
                continue;

            if ( /* FILL HERE: message from client 'frsock'? */ ) {
                char * clienthost; /* host name of the client */
                ushort clientport; /* port number of the client */


                /*
                 FILL HERE:
                 figure out client's host name and port
                 using getpeername() and gethostbyaddr()
                 */


                /* read the message */
                msg = recvtext(frsock);
                if (!msg) {
                    /* disconnect from client */
                    printf("admin: disconnect from '%s(%hu)'\n", clienthost,
                            clientport);

                    /*
                     FILL HERE:
                     remove this guy from the set of live clients
                     */


                    /* close the socket */
                    close(frsock);
                } else {
                    /*
                     FILL HERE
                     send the message to all live clients
                     except the one that sent the message
                     */


                    /* display the message */
                    printf("%s(%hu): %s", clienthost, clientport, msg);

                    /* free the message */
                    free(msg);
                }
            }
        }

        /* look for connect requests */
        if ( /* FILL HERE: connect request from a new client? */ ) {

            /*
             FILL HERE:
             accept a new connection request
             */


            /* if accept is fine? */
            if (csd != -1) {
                char * clienthost; /* host name of the client */
                ushort clientport; /* port number of the client */

                /*
                 FILL HERE:
                 figure out client's host name and port
                 using gethostbyaddr() and without using getpeername().
                 */

                printf("admin: connect from '%s' at '%hu'\n", clienthost,
                        clientport);

                /*
                 FILL HERE:
                 add this guy to set of live clients
                 */


            } else {
                perror("accept");
                exit(0);
            }
        }
    }
    return 0;
}
/*--------------------------------------------------------------------*/

