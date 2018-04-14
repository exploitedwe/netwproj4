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

//Is this finall working?
// Testing from laptops side now!

#define MAX 100 // Max clients, Max I/O input and output

void *handleClient();

int activeThreads = 0; //For checking how many threads are currently open.
//pthread__mutex_t activeTM;
//pthread_cond_t activeTC;

typedef struct param{
  int fileDescriptor;
} Param;

int main(int argc, char* argv[]){
  int     sk, maxfd, incrmnt, tmp, activity,
          clientAmnt, clientSockets[MAX];
  char    sendline[MAX], rcvline[MAX];
  struct  sockaddr_in skaddr;
  //struct timeval timeout; timeout.tv_sec = 35; //35s until timeout.
  fd_set  rset;

  if(argc != 2){
    fprintf(stderr, "Incorrect parameters-- ./server [PORT]\n");
    exit(-1);
  }

  // Socket Creation
  if( (sk = socket(PF_INET, SOCK_STREAM, 0)) < 0 ){
    fprintf(stderr, "Error creating socket.\n");
    exit(-1);
  }

  // Socket Struct Fill-in
  skaddr.sin_family       = AF_INET;
  skaddr.sin_addr.s_addr  = htonl(INADDR_ANY);
  skaddr.sin_port         = htons(atoi(argv[1]));


  // Socket Bind
  int skaddrSize = sizeof(skaddr);
  if( bind(sk, (struct sockaddr *) &skaddr, skaddrSize) < 0 ){
    fprintf(stderr, "Error binding socket.\n");
    exit(-1);
  }

  // Double-check Socket works, and print status to console
  if( getsockname(sk, (struct sockaddr *) &skaddr, &skaddrSize) < 0) {
    fprintf(stderr, "Error getting sock name.\n");
    exit(-1);
  } else printf("Server [ON]\tSocket [OPEN]\n");

  printf("SERVER DETAILS: IP:%s\tPORT:%d\n", inet_ntoa(skaddr.sin_addr), ntohs(skaddr.sin_port));

  // Start listening w/ backlog (queue) of 100
  if( listen(sk, 5) < 0){
      fprintf(stderr, "Error listening on socket.\n");
      exit(-1);
  }else printf("Waiting for connections...");



  // Server is now ready for clients

  while(1){
    FD_ZERO(&rset);     // Zero's fd_set
    FD_SET(sk, &rset);  // Adds master socket to fd_set
    //FD_SET(fileno(fp), &rset); // Adds file descriptor to fd_set

    // Cycles, finds max file descriptor
    for(incrmnt=0; incrmnt < clientAmnt; incrmnt++){
      tmp = clientSockets[incrmnt];
      if(tmp > maxfd) maxfd = tmp;
    }

    // Select indicates there is a file descriptor (rset) ready for reading
    if( (activity = select(sk, &rset, NULL, NULL, NULL)) < 0 )
      fprintf(stderr, "Error with select.\n");

    /* Took this from slide deck, doubt i need its
    if (FD_ISSET(sk, &rset)) {	// Socket is readable
			if (readline(sk, recvline, MAX) == 0)
        fprintf(stderr,"str_cli: server terminated prematurely");
			fputs(recvline, stdout);
		}

		if (FD_ISSET(fileno(fp), &rset)) {  // Input is readable
			if (fgets(sendline, MAX, fp) == NULL) return;
			writen(sk, sendline, strlen(sendline));
		}*/


    if(activity && FD_ISSET(sk, &rset)){ // Means select saw a file descriptor signal it's ready
      int sd;
      struct sockaddr_in from;
      sd = accept(sk, (struct sockaddr*) &from, &skaddrSize);
      pthread_t tid;
      pthread_create(&tid, NULL, &handleClient, &sd);
      printf("Client thread created.\n");
      pthread_join(tid, NULL);
      printf("Client thread joined.\n");

    }
  }

  return 0;
}
void *handleClient(int *fd){
  printf("Wow you made it this far! great job you!\n");
  exit(EXIT_SUCCESS);
}
