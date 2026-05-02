#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

int t_id[2]={1,2};
int count=0;
pthread_mutex_t mutex;

void *t_func(void *arg){
    int *id = (int *)arg;

    printf("Entered in Thread %d...\n", *id);

    int local = 0;
    for(int i=0;i<100000;i++){
        local++;
    }

    pthread_mutex_lock(&mutex);
    count += local;
    pthread_mutex_unlock(&mutex);

    return NULL;
}

int main(){
    pthread_t t[2];

    pthread_mutex_init(&mutex, NULL);

    pthread_create(&t[0], NULL, t_func, &t_id[0]);
    pthread_create(&t[1], NULL, t_func, &t_id[1]);

    for(int i=0;i<2;i++){
        pthread_join(t[i], NULL);
    }

    pthread_mutex_destroy(&mutex);

    printf("Total count: %d\n", count);
    return 0;
}