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
//pthread__mutex_t activeTM;
//pthread_cond_t activeTC;

typedef struct param{
  int fileDescriptor;
} Param;

int main(int argc, char* argv[]){
  int     sk, client_sk, maxfd, incrmnt, tmp, activity,
          clientAmnt, clientSockets[MAX];
  char    sendline[MAX], rcvline[MAX];
  struct  sockaddr_in sockaddr;


  //struct timeval timeout; timeout.tv_sec = 35; //35s until timeout.
  fd_set  rset;

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

  printf("SERVER DETAILS: IP:%s\tPORT:%d\n", inet_ntoa(sockaddr.sin_addr), ntohs(sockaddr.sin_port));

  // Start listening w/ backlog (queue) of 100
  if( listen(sk, 100) < 0){
      fprintf(stderr, "Error listening on socket.\n");
      exit(EXIT_FAILURE);
  }else printf("Waiting for connections...\n");


  // Server is now ready for clients
  for(tmp=0; tmp < MAX; tmp++) clientSockets[tmp] = 0;

  while(1){
    FD_ZERO(&rset);
    FD_SET(sk, &rset);
    maxfd = sk;
    int sd;
    pthread_t tid;


    for (tmp = 0 ; tmp < MAX; tmp++){
        sd = clientSockets[tmp];
        if(sd > 0) FD_SET(sd , &rset);
        if(sd > maxfd) maxfd = sd;
    }

    if(activity = select( maxfd + 1 , &rset , NULL , NULL , NULL) < 0){
      fprintf(stderr,"Error in select.\n");
      exit(EXIT_FAILURE);
    }

    if (activity < 0) printf("select error\n");

    if (FD_ISSET(sk, &rset)){
        if ((client_sk = accept(sk,
                (struct sockaddr *)&sockaddr, (socklen_t*)&skaddrSize))<0) {
            perror("accept");
            exit(EXIT_FAILURE);
        }

        pthread_create(&tid, NULL, &handleClient, &client_sk);

        // Adds client to master client list
        for (tmp = 0; tmp < MAX; tmp++){
            //if position is empty
            if( clientSockets[tmp] == 0 ){
                clientSockets[tmp] = client_sk;
                printf("Adding to list of sockets as %d\n" , tmp);
                break;
            }
        }
    }

    //else its some IO operation on some other socket
    /*for (tmp = 0; tmp < MAX; tmp++){
        sd = clientSockets[tmp];
        int valread;
        if (FD_ISSET( sd , &rset)){
            if ((valread = read( sd , buffer, 999)) == 0){
                getpeername(sd , (struct sockaddr*)&sockaddr , \
                    (socklen_t*)&skaddrSize);
                printf("Host disconnected , ip %s , port %d \n" ,
                      inet_ntoa(sockaddr.sin_addr) , ntohs(sockaddr.sin_port));

                close( sd );
                clientSockets[tmp] = 0;
            }else{
                buffer[valread] = '\0';
                send(sd , buffer , strlen(buffer) , 0 );
            }
        }
    }*/
  }

  return 0;
}
void *handleClient(int *client_sk){
  char* message = "HELLO";
  printf("New connection , socket fd is %d\n",*client_sk);
  //send new connection greeting message
  if( send(*client_sk, message, strlen(message), 0) != strlen(message) ){
      fprintf(stderr,"MessageSent\n");
  }
  puts("Welcome message sent successfully");

  sleep(100);
  printf("Exiting client: %d\n", *client_sk);
}
