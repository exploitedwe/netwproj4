/*
  Mitchell Weldon -- mmweldo@g.clemson.edu
  Networking & Network Programming: CPSC 3600
  Instructor

  Project/Homework: 4; Design a simple multi-threaded server.
  Due: April 19, 2018

  MISC: Reader, I used tab-size of 2, 4 might blow out comment blocks.
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
void reverse(char* line,int n);

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
    int client_sk, sd;
    pthread_t tid;

    client_sk = accept(sk, (struct sockaddr*)&sockaddr, (socklen_t*)&skaddrSize);

    pthread_mutex_lock(&clSockM);
    for (tmp = 0; clientSockets[tmp] != 0; tmp++);
    clientSockets[tmp] = client_sk;
    sd = tmp;
    pthread_mutex_unlock(&clSockM);

    // Struct for holding the clients needed inputs
    Param *param = malloc(sizeof(Param));// Allocating resources
    param->client_sk  = client_sk;       // Used for I/O
    param->index      = sd;               // Used for cleaning up after

    pthread_create(&tid, NULL, &handleClient, (void*) param);

    pthread_mutex_lock(&activeTM);
    activeThreads++;
    pthread_mutex_unlock(&activeTM);
    totalClients++;

    printf("Active Threads: %d\nTotal Clients: %d\n"
        ,activeThreads,totalClients);
  }

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
  int         i, j;        // Counter/wordcount, Counter
  size_t      buffsize;   // Buffer size, used across differetn buffers
  time_t      rawtime;    // Time handling
  struct tm   *timeinfo;   // Time handling
  char        *buff, *inpFileName, *outpFileName, // File buffers and names
              c, temp, ack[4];    // For reversing strings and ACK
  FILE        *inputFile = fdopen(param->client_sk,"r"), // Socket to File stream
              *outputFile; // Output file's file stream

  buff          = malloc(1000);
  inpFileName   = malloc(20);
  outpFileName  = malloc(35);

/* Receives file name from client -------------------------------------------
    My protocol for how the client and server interact starts with the client(s)
    sending their
---------------------------------------------------------------------------*/
  buffsize = sizeof(char) * 20;
  recv(param->client_sk, inpFileName, buffsize,0);

/* Time handling ----------------------------------------------------------
  Determines [local] time, used in naming an output file that won't
  conflict with other clients file names--hopefully.

  Format for "time_portion" of header: mm/dd/yyyy_hh:mm:ss --in military hours
---------------------------------------------------------------------------*/
  time(&rawtime);
  timeinfo = localtime(&rawtime);

  //sprintf is like printf/fprintf, but instead you can output to a char*
    //The '+1900' is due to tm_year only storing years since 1900. Odd.
  sprintf(outpFileName, "%d-%d-%d_%d:%d:%d_%s%s",
      timeinfo->tm_mon, timeinfo->tm_mday, timeinfo->tm_year+1900,
      timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec, inpFileName,".txt");

  outputFile = fopen(outpFileName, "w+b"); // Opens new file with name above
  if(!outputFile){
    fprintf(stderr, "fopen error\n");
    exit(EXIT_FAILURE);
  }

  buffsize = sizeof(char) * 1000;
  ack[0] = 'A'; ack[1] = 'C'; ack[2] = 'K'; ack[3] = '\0';

  send(param->client_sk, ack, sizeof(ack), 0);

  while(1){
    i = recv(param->client_sk, buff, buffsize, 0);
    if(strncmp(buff, "DONE--EOF",9) == 0) break;
    buff[i] = '\0';

    j = 0;
    for(;j < i/2;j++){
      c = buff[j];
      temp = buff[i-j-1];
      buff[i-j-1] = c;
      buff[j] = temp;
    }

    fputs(buff, outputFile);
    send(param->client_sk, ack, sizeof(ack), 0);
  }

  free(buff);
  free(inpFileName);
  free(outpFileName);

  // Closing NULL FILE* causes segfault.
  fclose(inputFile);
  fclose(outputFile);
  sleep(6);

  // In OS--CPSC 3220 we learn to release nested locks in reverse request order
  pthread_mutex_lock(&clSockM);
  pthread_mutex_lock(&activeTM);
  activeThreads--;
  clientSockets[param->index] = 0;
  printf("Done with connection, closing client_sk: %d\n", param->index);
  close(param->client_sk); //Closes socket
  free(param); // Free's the malloc() done in main() for thread parameters
  pthread_mutex_unlock(&activeTM);
  pthread_mutex_unlock(&clSockM);
}
void reverse(char* line,int n){
  int x = 0;
  char c;
  char temp;

  for(;x<n/2;x++){
    c = line[x];
    temp = line[n-x-1];
    line[n-x-1] = c;
    line[x] = temp;
  }
}
