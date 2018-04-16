#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>


int main(int argc, char* argv[]){
  int     sk;
  size_t  len = 500, read = 0;
  char*   message = malloc(500);
  struct  sockaddr_in skaddr;

  // Parameter Handling
  if(argc != 4){
    fprintf(stderr,"Incorrect parameters-- "
        "./client [Server IP] [PORT] [myfile.ext]\n");
    exit(-1);
  }

  // Socket Creation
  if( (sk = socket(PF_INET, SOCK_STREAM, 0)) < 0 ){
    fprintf(stderr, "Error Creating socket.\n");
    exit(-1);
  }

  // IP/Port Assignment
  skaddr.sin_family = AF_INET;
  skaddr.sin_addr.s_addr = inet_addr(argv[1]);
  skaddr.sin_port = htons(atoi(argv[2]));

  //Check valid
  if(skaddr.sin_addr.s_addr == -1){
    printf("Invalid IP Address: %s\n", argv[1]);
    exit(-1);
  }

  // Server Connect
  if(connect(sk, (struct sockaddr *) &skaddr, sizeof(skaddr)) < 0) {
    printf("Problem connecting to server.\n");
    exit(-1);
  }

  FILE* fp = fopen(argv[3],"r");
  // Send file name, first string read by server, used in naming output file
  write(sk, argv[3], strlen(argv[3]));

  // I used send() and recvfrom() for my ACK communication
  recv(sk, message, sizeof(message), 0);
  if(strncmp(message, "ACK",3) != 0){
    fprintf(stderr, "Error: didn't receive first ACK. recv'd:%s\n", message);
    exit(EXIT_FAILURE);
  }

  while((read = getline(&message, &len, fp)) != -1){
    send(sk, message, strlen(message), 0);
    recv(sk, message, sizeof(message), 0);
    if(strncmp(message, "ACK",3) != 0){
      fprintf(stderr, "Error: didn't receive first ACK. recv'd:%s\n", message);
      exit(EXIT_FAILURE);
    }
  }
  
  send(sk, "DONE--EOF", strlen("DONE--EOF"), 0);
  close(sk);
  if(message != NULL) free(message);
  if(fp != NULL) fclose(fp);
  return 0;
}
