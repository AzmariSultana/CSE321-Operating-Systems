#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>

int t_id[4]={1,2,3,4};
int count=0;
sem_t s;

void *t_func(void *arg){
    int *id = (int *)arg;

    printf("Entered in Thread %d...\n", *id);

    int local = 0;
    for(int i=0;i<100000;i++){
        local++;
    }

    sem_wait(&s);
    count += local;
    sem_post(&s);

    return NULL;
}

int main(){
    pthread_t t[4];
    sem_init(&s,0,1);

    for(int i=0;i<4;i++){
        pthread_create(&t[i], NULL, t_func, &t_id[i]);
    }

    for(int i=0;i<4;i++){
        pthread_join(t[i], NULL);
    }

    sem_destroy(&s);

    printf("Total count: %d\n", count);
    return 0;
}