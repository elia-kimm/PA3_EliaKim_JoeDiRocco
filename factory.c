//---------------------------------------------------------------------
// Assignment : PA-03 UDP Single-Threaded Server
// Date       : Nov 12th 2025
// Author     : Joe DiRocco - Elia Kim
// File Name  : factory.c
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

#define MAXSTR     200
#define IPSTRLEN    50

typedef struct sockaddr SA ;

int minimum( int a , int b)
{
    return ( a <= b ? a : b ) ; 
}

void subFactory( int factoryID , int myCapacity , int myDuration ) ;

void factLog( char *str )
{
    printf( "%s" , str );
    fflush( stdout ) ;
}

/*-------------------------------------------------------*/

// Global Variable for Future Thread to Shared
int   remainsToMake , // Must be protected by a Mutex
      actuallyMade ;  // Actually manufactured items

int   numActiveFactories = 1 , orderSize ;

int   sd ;      // Server socket descriptor
struct sockaddr_in  
             srvrSkt,       /* the address of this server   */
             clntSkt;       /* remote client's socket       */
socklen_t clntLen;

//------------------------------------------------------------
//  Handle Ctrl-C or KILL 
//------------------------------------------------------------
void goodbye(int sig) 
{
    /* Mission Accomplished */
    printf( "\n### I (%d) have been nicely asked to TERMINATE. "
           "goodbye\n\n" , getpid() );  

    close(sd);
    exit(EXIT_SUCCESS);
}

/*-------------------------------------------------------*/
int main( int argc , char *argv[] )
{
    char  *myName = "Joe DiRocco - Elia Kim" ; 
    unsigned short port = 50015 ;      /* service port number  */
    int    N = 1 ;                     /* Num threads serving the client */

    printf("\nThis is the FACTORY server developed by %s\n\n" , myName ) ;
    char myUserName[30] ;
    getlogin_r ( myUserName , 30 ) ;
    time_t  now;
    time( &now ) ;
    fprintf( stdout , "Logged in as user '%s' on %s\n\n" , myUserName ,  ctime( &now)  ) ;
    fflush( stdout ) ;

	switch (argc) 
	{
      case 1:
        break ;     // use default port with a single factory thread
      
      case 2:
        N = atoi( argv[1] ); // get from command line
        port = 50015;            // use this port by default
        break;

      case 3:
        N    = atoi( argv[1] ) ; // get from command line
        port = atoi( argv[2] ) ; // use port from command line
        break;

      default:
        printf( "FACTORY Usage: %s [numThreads] [port]\n" , argv[0] );
        exit( 1 ) ;
    }

    // missing code goes here

    //creating UDP socket
    if ((sd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        err_sys("Could NOT create socket");
    }

    // signal handlers
    sigactionWrapper(SIGINT, goodbye);
    sigactionWrapper(SIGTERM, goodbye);

    //prepare the servers socket address structure
    memset((void *) &srvrSkt, 0, sizeof(srvrSkt));
    srvrSkt.sin_family = AF_INET;
    srvrSkt.sin_addr.s_addr = htonl(INADDR_ANY);
    srvrSkt.sin_port = htons(port);

    // now bind the server to above socket
    if ( bind( sd, (SA *) & srvrSkt , sizeof(srvrSkt) ) < 0 )
    {
        err_sys("bind failed");
    }

    //turn the ip into a human readable string
    char ipStr[IPSTRLEN];
    inet_ntop( AF_INET, (void *) & srvrSkt.sin_addr.s_addr , ipStr , IPSTRLEN ) ;
    //print socket number, IP, and port
    printf("Bound socket %d to IP %s Port %u\n",
        sd, ipStr, ntohs(srvrSkt.sin_port));

    msgBuf msg1;

    int forever = 1;
    while ( forever )
    {
        printf( "\nFACTORY server waiting for Order Requests\n" ) ; 

        clntLen = sizeof(clntSkt);

        // receive REQUEST_MSG
        if (recvfrom(sd, &msg1, sizeof(msg1), 0, (SA *)&clntSkt, &clntLen) < 0) {
            err_sys("recvfrom failed");
        }

        printf("\n\nFACTORY server received: " ) ;
        printMsg( & msg1 );  puts("");

        if ( ntohl(msg1.purpose) != REQUEST_MSG) {
            printf("ERROR: not a request message");
            continue;
        }

        orderSize = ntohl(msg1.orderSize);
        remainsToMake = orderSize;
        actuallyMade = 0;
        numActiveFactories = 1;


        // send order_confirm
        msg1.purpose = htonl(ORDR_CONFIRM);
        msg1.numFac = htonl(1);

        if (sendto(sd, &msg1, sizeof(msg1), 0, (SA *)&clntSkt, clntLen) < 0) {
            err_sys("sendto failed");
        }

        printf("\n\nFACTORY sent this Order Confirmation to the client " );
        printMsg(  & msg1 );  puts("");
        
        subFactory( 1 , 50 , 350 ) ;  // Single factory, ID=1 , capacity=50, duration=350 ms
    }


    return 0 ;
}

void subFactory( int factoryID , int myCapacity , int myDuration )
{
    char    strBuff[ MAXSTR ] ;   // snprint buffer
    int     partsImade = 0 , myIterations = 0 ;
    msgBuf  msg;

    while ( 1 )
    {
        // See if there are still any parts to manufacture
        if ( remainsToMake <= 0 )
            break ;   // Not anymore, exit the loop
        


        // missing code goes here
        // choose how many to make
        int numCurMaking = minimum(myCapacity, remainsToMake);
        remainsToMake -= numCurMaking;
        partsImade += numCurMaking;
        myIterations++;

        //sleep to simulate production
        Usleep(myDuration * 1000);



        // Send a Production Message to Supervisor
        msg.purpose = htonl(PRODUCTION_MSG);
        msg.facID = htonl(factoryID);
        msg.capacity = htonl(myCapacity);
        msg.partsMade = htonl(numCurMaking);
        msg.duration = htonl(myDuration);

        if(sendto(sd, &msg, sizeof(msg), 0, (SA *)&clntSkt, clntLen) < 0 ) {
            err_sys("sendto PRODUCTION_MSG failed");
        }


    }

    // Send a Completion Message to Supervisor


    msg.purpose = htonl(COMPLETION_MSG);
    msg.facID = htonl(factoryID);

    if (sendto(sd, &msg, sizeof(msg), 0, (SA *)&clntSkt, clntLen) < 0) {
        err_sys("sendto COMPLETION_MSG failed");
    }


    snprintf( strBuff , MAXSTR , ">>> Factory # %-3d: Terminating after making total of %-5d parts in %-4d iterations\n" 
          , factoryID, partsImade, myIterations);
    factLog( strBuff ) ;
    
}

