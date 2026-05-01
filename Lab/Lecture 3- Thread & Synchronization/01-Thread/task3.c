#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

void *func_thread(void *arg);

int main() {
    pthread_t t1;
    int num = 5;
    void *t_ret;

    pthread_create(&t1, NULL, func_thread, &num);
    pthread_join(t1, &t_ret);

    int result = *(int *)t_ret;
    printf("Thread returned: %d\n", result);

    free(t_ret);  // free allocated memory

    return 0;
}

void *func_thread(void *arg) {
    int *n = (int *)arg;

    int *result = malloc(sizeof(int));  // allocate memory for return value

    printf("Entered in Thread:\n");

    if (*n % 2 == 0) {
        *result = (*n) * (*n);
    } else {
        *result = (*n) * (*n) * (*n);
    }

    printf("Operation completed\n");

    return (void *)result;
}