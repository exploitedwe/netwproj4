#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <pthread.h>

#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>   // socket()
#include <sys/types.h>
#include <sys/select.h>   //select()
#include <sys/time.h>
#include <string.h>

void* pthreadFunc();
int fd = 0;
char buff[11];
int ret, sret;
fd_set readfds;
struct timeval timeout;

int main(int arvc, char* argv[]){


  while(1){
    FD_ZERO(&readfds);
    FD_SET(fd, &readfds);
    pthread_t tid;

    timeout.tv_sec = 5;
    timeout.tv_usec = 0;

    sret = select(8, &readfds, NULL, NULL, &timeout);
    if(sret == 0){
      printf("sret = %d\n", sret);
      printf("   timeout\n");
    }
    else{
      pthread_create(&tid, NULL, &pthreadFunc, NULL);
      printf("Thread created.\n");
      pthread_join(tid, NULL);
    }
  }

  return 0;
}
void* pthreadFunc(){
  memset( (void *) buff, 0, 11);
  ret = read(fd, (void *)buff, 10);
  printf("ret = %d\n", ret);

  if(ret != -1){
    printf(" buff=%s\n", buff);
  }

  printf("Thread completed.\n");
  return 0;
}
