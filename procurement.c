//---------------------------------------------------------------------
// Assignment : PA-03 UDP Single-Threaded Server
// Date       : Nov 12th 2025
// Author     : Joe DiRocco - Elia Kim
// File Name  : procurement.c
//---------------------------------------------------------------------

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include "wrappers.h"
#include "message.h"

#define MAXFACTORIES    20

typedef struct sockaddr SA ;

/*-------------------------------------------------------*/
int main( int argc , char *argv[] )
{
    int     numFactories ,      // Total Number of Factory Threads
            activeFactories ,   // How many are still alive and manufacturing parts
            iters[ MAXFACTORIES+1 ] = {0} ,  // num Iterations completed by each Factory
            partsMade[ MAXFACTORIES+1 ] = {0} , totalItems = 0;

    char  *myName = "DiRocco-Kim" ; 
    printf("\nPROCUREMENT: Started. Developed by %s\n\n" , myName );    

    char myUserName[30] ;
    getlogin_r ( myUserName , 30 ) ;
    time_t  now;
    time( &now ) ;
    fprintf( stdout , "Logged in as user '%s' on %s\n\n" , myUserName ,  ctime( &now)  ) ;
    fflush( stdout ) ;
    
    if ( argc < 4 )
    {
        printf("PROCUREMENT Usage: %s  <order_size> <FactoryServerIP>  <port>\n" , argv[0] );
        exit( -1 ) ;  
    }

    unsigned        orderSize  = atoi( argv[1] ) ;
    char	       *serverIP   = argv[2] ;
    unsigned short  port       = (unsigned short) atoi( argv[3] ) ;
 

    /* Set up local and remote sockets */
    int sd;
    if ((sd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        err_sys("Could NOT create socket");
    }

    // Prepare the server's socket address structure
    struct sockaddr_in srvrSkt;
    memset((void *) &srvrSkt, 0, sizeof(srvrSkt));
    srvrSkt.sin_family = AF_INET;
    srvrSkt.sin_port = htons(port);
    if (inet_pton(AF_INET, serverIP, (void *) &srvrSkt.sin_addr.s_addr) != 1) {
        err_sys("Invalid server IP address");
    }
    socklen_t srvLen = sizeof(srvrSkt);


    // Send the initial request to the Factory Server
    msgBuf  msg1;
    msg1.purpose = htonl(REQUEST_MSG);
    msg1.orderSize = htonl(orderSize);

    if (sendto(sd, &msg1, sizeof(msg1), 0, (SA *)&srvrSkt, srvLen) < 0) {
        err_sys("sendto request failed");
    }


    printf("\nPROCUREMENT Sent this message to the FACTORY server: "  );
    printMsg( & msg1 );  puts("");


    /* Now, wait for oreder confirmation from the Factory server */
    msgBuf  msg2;
    printf ("\nPROCUREMENT is now waiting for order confirmation ...\n" );

    if(recvfrom(sd, &msg2, sizeof(msg2), 0, (SA *)&srvrSkt, &srvLen) < 0) {
        err_sys("recvfrom failed");
    }



    printf("PROCUREMENT received this from the FACTORY server: "  );
    printMsg( & msg2 );  puts("\n");

    if (ntohl(msg2.purpose) != ORDR_CONFIRM) {
        err_quit("expected ORDR_CONFIRM\n");
    }
    
    numFactories = ntohl(msg2.numFac);
    activeFactories = numFactories;


    // Monitor all Active Factory Lines & Collect Production Reports
    while ( activeFactories > 0 ) // wait for messages from sub-factories
    {
        msgBuf msg;
        if (recvfrom(sd, &msg, sizeof(msg), 0, (SA *)&srvrSkt, srvLen) < 0) {
            err_sys("recvfrom failed\n");
        }


        // Inspect the incoming message
        int purpose = ntohl(msg.purpose);
        if (purpose == PRODUCTION_MSG) {
            int id = ntohl(msg.facID);
            printf("PROCUREMENT: ");
            printMsg(&msg);
            puts("");
            iters[id]++;
            partsMade[id] += ntohl(msg.partsMade);
        } else if (purpose == COMPLETION_MSG) {
            int id = ntohl(msg.facID);
            printf("PROCUREMENT: Factory #%d       COMPLETED its task\n", msg.facID);
            activeFactories--;
        } else {
            printf("PROCUREMENT: Received invalid msg { PROTOCOL_ERROR }\n");
        }
    } 

    // Print the summary report
    totalItems  = 0 ;
    printf("\n\n****** PROCUREMENT Summary Report ******\n");


    for (int i = 1; i <= numFactories; i++) {
        printf("Factory #%d: iterations=%d, partsMade=%d\n",
            i, iters[i], partsMade[i]);
        totalItems += partsMade[i];
    }


    printf("==============================\n") ;


    printf("Grand total parts made =  %d   vs   order size of   %d\n",
        totalItems, orderSize);


    printf( "\n>>> PROCUREMENT Terminated\n");



    close(sd);

    return 0 ;
}
