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

int main(int arvc, char* argv[]){
  int fd;
  char buff[11];
  int ret, sret;

  fd = 0;

  fd_set readfds;
  struct timeval timeout;

  while(1){
    FD_ZERO(&readfds);
    FD_SET(fd, &readfds);

    timeout.tv_sec = 5;
    timeout.tv_usec = 0;

    sret = select(8, &readfds, NULL, NULL, NULL);
    if(sret == 0){
      printf("sret = %d\n", sret);
      printf("   timeout\n");
    }
    else{
      memset( (void *) buff, 0, 11);
      ret = read(fd, (void *)buff, 10);
      printf("ret = %d\n", ret);

      if(ret != -1){
        printf(" buff=%s\n", buff);
      }
    }
  }

  return 0;
}
