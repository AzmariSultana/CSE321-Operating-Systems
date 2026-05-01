#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

void *func_thread(void *arg);

int main() {
    pthread_t t1;
    int n;
    void *t_ret;

    printf("Enter the size of the array:\n");
    scanf("%d", &n);

    pthread_create(&t1, NULL, func_thread, &n);
    pthread_join(t1, &t_ret);

    int *arr = (int *)t_ret;

    for (int i = 0; i < n; i++) {
        printf("a[%d]: %d\n", i, arr[i]);
    }

    free(arr);

    return 0;
}

void *func_thread(void *arg) {
    int *v = (int *)arg;

    int *a = malloc(sizeof(int) * (*v));
    if (a == NULL) {
        pthread_exit(NULL);
    }

    for (int i = 0; i < *v; i++) {
        printf("Enter value in a[%d]:\n", i);
        scanf("%d", &a[i]);
    }

    return (void *)a;
}