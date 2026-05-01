#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

void *func_thread(void *arg);

int main(){
    pthread_t t1;
    int n = 5;
    void *t_ret;

    pthread_create(&t1, NULL, func_thread, &n);
    pthread_join(t1, &t_ret);

    printf("Thread returned: %d\n", *(int*)t_ret);

    return 0;
}

void *func_thread(void *arg){
    int *v = (int *)arg; 
    *v = (*v) * 5;

    return (void *)v;
}