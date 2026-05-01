#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

typedef struct {
    int tid;
    int a;
    int b;
} thread_data;

void *t_func1(void *arg);
void *t_func2(void *arg);

int main() {
    pthread_t t1, t2;

    thread_data *data1 = malloc(sizeof(thread_data));
    thread_data *data2 = malloc(sizeof(thread_data));

    data1->tid = 1;
    data1->a = 10;
    data1->b = 5;

    data2->tid = 2;
    data2->a = 10;
    data2->b = 5;

    pthread_create(&t1, NULL, t_func1, data1);
    pthread_create(&t2, NULL, t_func2, data2);

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    free(data1);
    free(data2);

    return 0;
}

void *t_func1(void *arg) {
    thread_data *data = (thread_data *)arg;
    printf("Entered in Thread: %d\n", data->tid);
    sleep(1);
    int add = data->a + data->b;
    printf("ADD: %d\n", add);
    printf("Addition Done by Thread %d...\n", data->tid);
    return NULL;
}

void *t_func2(void *arg) {
    thread_data *data = (thread_data *)arg;
    printf("Entered in Thread: %d\n", data->tid);
    sleep(1);
    int sub = data->a - data->b;
    printf("SUB: %d\n", sub);
    printf("Subtraction Done by Thread %d...\n", data->tid);
    return NULL;
}