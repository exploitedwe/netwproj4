#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>

int main(int argc, char* argv[]){
  int sk;
  char* message = "Did I make it?";
  struct sockaddr_in skaddr;

  // Parameter Handling
  if(argc != 4){
    fprintf(stderr,"Incorrect parameters-- ./client [Server IP] [PORT] [myfile.ext]\n");
    exit(-1);
  }

  // Socket Creation
  if( (sk = socket(PF_INET, SOCK_STREAM, 0)) < 0 ){
    fprintf(stderr, "Error Creating socket.\n");
    exit(-1);
  }

  // IP Assignment and Check
  skaddr.sin_addr.s_addr = inet_addr(argv[1]);
  if(skaddr.sin_addr.s_addr == -1){
    printf("Invalid IP Address: %s\n", argv[1]);
    exit(-1);
  }

  // Port Assignment
  skaddr.sin_port = htons(atoi(argv[2]));

  // Server Connect
  if(connect(sk, (struct sockaddr *) &skaddr, sizeof(skaddr)) < 0) {
    printf("Problem connecting to server.\n");
    exit(-1);
  }

  // Send a single string
  write(sk, message, strlen(message));

  close(sk);
  return 0;
}
