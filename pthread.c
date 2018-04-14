#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <pthread.h>

void* pthreadFunc();

int main(int argc, char* argvp[]){
  pthread_t tid;


  /* pthread_create header and parameters
    pthread_create(
        pthread_t *tid,
        const pthread_attr_t *attr,
        void *(*func) (void *),
        void * arg);
  */
  pthread_create(&tid, NULL, &pthreadFunc, NULL);
  pthread_join(tid, NULL);
  printf("Threada joined.\n");
  return 0;
}
void* pthreadFunc(){
  printf("Thread [%d] Created.\n", (int)pthread_self());
  return 0;
}
