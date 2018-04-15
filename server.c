/*
  Mitchell Weldon -- mmweldo@g.clemson.edu
  Networking & Network Programming: CPSC 3600
  Instructor


  Project/Homework: 4; Design a simple multi-threaded server.
  Due: April 19, 2018
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>   // socket()
#include <sys/types.h>
#include <sys/select.h>   //select()
#include <sys/time.h>
#include <string.h>
#include <pthread.h>

#define MAX 100 // Max clients, Max I/O input and output

void *handleClient();

// Global so threads can access + their locks to avoid conflict
int activeThreads = 0;     // For checking how many threads are currently open.
pthread_mutex_t activeTM;  // Lock for accessing activeThreads
//pthread_cond_t activeTC; // Cond var for checking if can access aT

int clientSockets[MAX];     // Array of CURRENT clients
pthread_mutex_t clSockM;    // Lock for accessing clientSockets[]
//pthread_cond_t clSockC;

/*    The handleClient() functions parameter struct is the only way to pass
multiple parameters to a thread, pthread_create only allows one arg option. */
typedef struct param{
  int client_sk;  // Socket. Same as client_sk assigned by accept()
  int index;      // The index of client_sk within clientSockets[]
} Param;

int main(int argc, char* argv[]){
  int     sk, client_sk, maxfd, i, tmp, activity, totalClients;
  struct  sockaddr_in sockaddr; // Server socket
  fd_set  rset; // Reading set of file descriptors, used in select()

  if(argc != 2){
    fprintf(stderr, "Incorrect parameters-- ./server [PORT]\n");
    exit(-1);
  }

  // Socket Creation
  if( (sk = socket(PF_INET, SOCK_STREAM, 0)) < 0 ){
    fprintf(stderr, "Error creating socket.\n");
    exit(EXIT_FAILURE);
  }

  // Socket struct fill-in
  sockaddr.sin_family       = AF_INET;
  sockaddr.sin_addr.s_addr  = htonl(INADDR_ANY);
  sockaddr.sin_port         = htons(atoi(argv[1]));

  // Socket Bind
  if( bind(sk, (struct sockaddr *) &sockaddr, sizeof(sockaddr)) < 0 ){
    fprintf(stderr, "Error binding socket.\n");
    exit(EXIT_FAILURE);
  }

  // Double-check socket works, and print status to console
  int skaddrSize = sizeof(sockaddr);
  if( getsockname(sk, (struct sockaddr *) &sockaddr, &skaddrSize) < 0) {
    fprintf(stderr, "Error getting sock name.\n");
    exit(EXIT_FAILURE);
  } else printf("Server [ON]\tSocket [OPEN]\n");

  printf("SERVER DETAILS: IP:%s\tPORT:%d\n",
          inet_ntoa(sockaddr.sin_addr), ntohs(sockaddr.sin_port));

  // Start listening w/ backlog (queue) of at most 100
  if( listen(sk, 100) < 0){
      fprintf(stderr, "Error listening on socket.\n");
      exit(EXIT_FAILURE);
  }else printf("Waiting for connections...\n");

  // Zero's client list, used for understanding which slots are OPEN in select
  for(tmp=0; tmp < MAX; tmp++) clientSockets[tmp] = 0;
  totalClients = 0; // Counter of total clients serviced

  // Server is now ready for clients
  while(1){
    FD_ZERO (&rset);
    FD_SET  (sk, &rset);

    maxfd =     sk;
    int         sd;
    pthread_t   tid; // pthread identifier

    // select needs maxfd as first parameter, this finds the maxfd
    for (tmp = 0 ; tmp < MAX; tmp++){
      sd = clientSockets[tmp];
      if(sd > 0) FD_SET(sd , &rset);
      if(sd > maxfd) maxfd = sd;
    }

    // Activity notes a change on the reading set, i.e. new client
    if(activity = select( maxfd + 1 , &rset , NULL , NULL , NULL) < 0){
      fprintf(stderr,"Error in select.\n");
      exit(EXIT_FAILURE);
    }

    if (FD_ISSET(sk, &rset)){
        // Accept
        if ((client_sk = accept(sk, (struct sockaddr *)&sockaddr,
                (socklen_t*)&skaddrSize)) < 0) {
          perror("accept");
          exit(EXIT_FAILURE);
        }

        // Adds client to master client list
        pthread_mutex_lock(&clSockM);
        for (i = 0; i < MAX; i++){
            //if position is empty
            if( clientSockets[i] == 0 ){
              clientSockets[i] = client_sk;
              printf("Adding to list of sockets as %d\n" , i);
              break;
            }
        }
        pthread_mutex_unlock(&clSockM);

        // Struct for holding the clients needed inputs
        Param *param = malloc(sizeof(Param));// Allocating resources
        param->client_sk  = client_sk;       // Used for I/O
        param->index      = i;               // Used for cleaning up after

        // Create thread for handling I/O, pass it it's parameters
        pthread_create(&tid, NULL, &handleClient, (void*) param);
        pthread_mutex_lock(&activeTM);
        activeThreads++;
        pthread_mutex_unlock(&activeTM);
        totalClients++;

        printf("Active Threads: %d\nTotal Clients: %d\n"
            ,activeThreads,totalClients);
    } // End FD_ISSET
  }// End while(1)

  return 0;
}
/* Threads Code block --------------------------------------------------------
  So the client has already been accepted, on accept, a client socket--client_sk
  was created. This functions objective is to communicate with the ./client(s)
  indicated by their client_sk. client_sk is within the passed struct Param
  *param.

  Output: a file with [time_portion][filename.ext].txt as output.
  ex.  3/15/2018_17:49:18_README.md.txt

  Within the file will be reversed data from what was originally within the
  clients version of the file.

  I kept the original extension for the filename, just in case changing
  the extension to .txt would somehow corrupt or ruin data. This way the server
  side can at least know what the original file extension is.
----------------------------------------------------------------------------*/
void *handleClient(Param *param){
  int         i;  // Counter
  size_t      buffsize;   // Buffer size, used across differetn buffers
  time_t      rawtime;    // Time handling
  struct tm*  timeinfo;   // Time handling
  char        buff[1000], *inpFileName, *outpFileName;
  FILE*       inputFile = fdopen(param->client_sk,"r"), // Socket to File stream
              outputFile; // Output file's file stream

  inpFileName   = malloc(20); // Using array didn't work, so I converted them
  outpFileName  = malloc(35); //    to simple malloc+free pointers.

/* Receives file name from client -------------------------------------------
    My protocol for how the client and server interact starts with the client(s)
    sending their
---------------------------------------------------------------------------*/
  buffsize = sizeof(char) * 20;
  i = getline(&inpFileName, &buffsize, inputFile);
  inpFileName[i] = '\0'; // Add termination character, just to be sure
  printf("File Name Received: %s\n", inpFileName);


/* Time handling ----------------------------------------------------------
  Determines [local] time, used in naming an output file that won't
  conflict with other clients file names--hopefully.

  Format for "time_portion" of header: mm/dd/yyyy_hh:mm:ss --in military hours
---------------------------------------------------------------------------*/
  time(&rawtime);
  timeinfo = localtime(&rawtime);

  //sprintf is like printf/fprintf, but instead you can output to a char*
    //The '+1900' is due to tm_year only storing years since 1900. Odd.
  sprintf(outpFileName, "%d/%d/%d_%d:%d:%d_%s",
      timeinfo->tm_mon, timeinfo->tm_mday, timeinfo->tm_year+1900,
      timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec, inpFileName);







  free(inpFileName);
  free(outpFileName);

  // In OS--CPSC 3220 we learn to release nested locks in reverse request order
  pthread_mutex_lock(&clSockM);
  pthread_mutex_lock(&activeTM);
  activeThreads--;
  clientSockets[param->index] = 0;
  printf("Done with connection, closing client_sk: %d\n", param->index);
  free(param);
  pthread_mutex_unlock(&activeTM);
  pthread_mutex_unlock(&clSockM);
}
