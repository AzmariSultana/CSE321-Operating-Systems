#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

typedef struct {
    int n;
    long long *fib;
} FibGenData;

typedef struct {
    long long *fib; 
    int n; 
    int *queries; 
    int queryCount;
    long long *results;
} FibSearchData;

void *generateFibonacci(void *arg) {
    FibGenData *data = (FibGenData *)arg;
    int n = data->n;

    data->fib = (long long *)malloc((n + 1) * sizeof(long long));
    if (data->fib == NULL) {
        fprintf(stderr, "Memory allocation failed!\n");
        pthread_exit(NULL);
    }

    if (n >= 0) 
        data->fib[0] = 0;
    if (n >= 1) 
        data->fib[1] = 1;
    for (int i = 2; i <= n; i++) {
        data->fib[i] = data->fib[i - 1] + data->fib[i - 2];
    }

    pthread_exit(NULL);
}

void *searchFibonacci(void *arg) {
    FibSearchData *data = (FibSearchData *)arg;

    for (int i = 0; i < data->queryCount; i++) {
        int idx = data->queries[i];
        if (idx >= 0 && idx <= data->n) {
            data->results[i] = data->fib[idx];
        } else {
            data->results[i] = -1;
        }
    }

    pthread_exit(NULL);
}

int main() {
    int n;
    printf("Enter the term of fibonacci sequence:\n");
    if (scanf("%d", &n) != 1) {
        printf("Invalid input! Please enter an integer.\n");
        return 1;
    }
    if (n < 0 || n > 40) {
        printf("Invalid input! n must be between 0 and 40.\n");
        return 1;
    }

    FibGenData genData;
    genData.n = n;
    genData.fib = NULL;

    pthread_t genThread;
    pthread_create(&genThread, NULL, generateFibonacci, &genData);
    pthread_join(genThread, NULL); 

    for (int i = 0; i <= n; i++) {
        printf("a[%d] = %lld\n", i, genData.fib[i]);
    }

    int s;
    printf("How many numbers you are willing to search?:\n");
    if (scanf("%d", &s) != 1) {
        printf("Invalid input! Please enter an integer.\n");
        free(genData.fib);
        return 1;
    }
    if (s <= 0) {
        printf("Invalid input! The number of searches must be greater than 0.\n");
        free(genData.fib);
        return 1;
    }

    int *queries = (int *)malloc(s * sizeof(int));
    for (int i = 0; i < s; i++) {
        printf("Enter search %d:\n", i + 1);
        if (scanf("%d", &queries[i]) != 1) {
            printf("Invalid input! Search indices must be integers.\n");
            free(genData.fib);
            free(queries);
            return 1;
        }
        int c = getchar();
        if (c == '.') {
            printf("Invalid input! Search indices must be integers.\n");
            free(genData.fib);
            free(queries);
            return 1;
        }
        if (c != '\n' && c != EOF) {
            ungetc(c, stdin);
        }
    }

    FibSearchData searchData;
    searchData.fib = genData.fib;
    searchData.n = n;
    searchData.queries = queries;
    searchData.queryCount = s;
    searchData.results = (long long *)malloc(s * sizeof(long long));

    pthread_t searchThread;
    pthread_create(&searchThread, NULL, searchFibonacci, &searchData);
    pthread_join(searchThread, NULL);

    for (int i = 0; i < s; i++) {
        printf("result of search #%d = %lld\n", i + 1, searchData.results[i]);
    }

    free(genData.fib);
    free(queries);
    free(searchData.results);

    return 0;
}