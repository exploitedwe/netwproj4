#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
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

int activeThreads = 0; //For checking how many threads are currently open.
pthread_mutex_t activeTM;  // Lock for accessing activeThreads
pthread_cond_t activeTC;    // Cond var for checking if can access aT

int clientSockets[MAX];
pthread_mutex_t clSockM;
pthread_cond_t clSockC;

typedef struct param{
  int client_sk;
  int index;
  //int* clientSockArry;
} Param;

int main(int argc, char* argv[]){
  int     sk, client_sk, maxfd, i, tmp, activity, totalClients;
  //char    sendline[MAX], rcvline[MAX];
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

  // Socket Struct Fill-in
  sockaddr.sin_family       = AF_INET;
  sockaddr.sin_addr.s_addr  = htonl(INADDR_ANY);
  sockaddr.sin_port         = htons(atoi(argv[1]));

  // Socket Bind
  if( bind(sk, (struct sockaddr *) &sockaddr, sizeof(sockaddr)) < 0 ){
    fprintf(stderr, "Error binding socket.\n");
    exit(EXIT_FAILURE);
  }

  // Double-check Socket works, and print status to console
  int skaddrSize = sizeof(sockaddr);
  if( getsockname(sk, (struct sockaddr *) &sockaddr, &skaddrSize) < 0) {
    fprintf(stderr, "Error getting sock name.\n");
    exit(EXIT_FAILURE);
  } else printf("Server [ON]\tSocket [OPEN]\n");

  printf("SERVER DETAILS: IP:%s\tPORT:%d\n",
          inet_ntoa(sockaddr.sin_addr), ntohs(sockaddr.sin_port));

  // Start listening w/ backlog (queue) of 100
  if( listen(sk, 100) < 0){
      fprintf(stderr, "Error listening on socket.\n");
      exit(EXIT_FAILURE);
  }else printf("Waiting for connections...\n");

  // Zero's client list, used for understanding which slots are OPEN
  for(tmp=0; tmp < MAX; tmp++) clientSockets[tmp] = 0;
  totalClients = 0;

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

        printf("Active Threads: %d\nTotal Clients: %d\n",activeThreads,totalClients);
    } // End FD_ISSET
  }// End while(1)

  return 0;
}
void *handleClient(Param *param){
  int i;
  //printf("New connection , socket fd is %d\n", param->client_sk);

  sleep(8); //Turn on/off to test multi-client

  // In OS--CPSC 3220 we learn to release nested locks in reverse request order
  pthread_mutex_lock(&clSockM);
  pthread_mutex_lock(&activeTM);
  activeThreads--;
  clientSockets[param->index] = 0;
  pthread_mutex_unlock(&activeTM);
  pthread_mutex_unlock(&clSockM);
  printf("Exiting clientSock: %d\n", param->index);
  free(param);
}
