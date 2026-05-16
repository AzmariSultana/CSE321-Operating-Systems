#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>

static const char *combos[] = {
    "Bread and Cheese",
    "Cheese and Lettuce",   
    "Bread and Lettuce"
};

sem_t supplierSem;
sem_t makerSem[3];
pthread_mutex_t tableMutex = PTHREAD_MUTEX_INITIALIZER;

int rounds; 

void *supplier(void *arg) {
    (void)arg;
    for (int i = 0; i < rounds; i++) {
        int combo = rand() % 3;

        pthread_mutex_lock(&tableMutex);
        printf("Supplier places: %s\n", combos[combo]);
        pthread_mutex_unlock(&tableMutex);

        int target;
        if (combo == 0) 
            target = 2;
        else if (combo == 1) 
            target = 0;
        else 
            target = 1;

        sem_post(&makerSem[target]);

        sem_wait(&supplierSem);
    }
    return NULL;
}

typedef struct {
    int id; 
    const char *name;
} MakerInfo;

void *maker(void *arg) {
    MakerInfo *info = (MakerInfo *)arg;

    while (1) {
        sem_wait(&makerSem[info->id]);

        const char *ingredients;
        if (info->id == 0)      
            ingredients = "Cheese and Lettuce";
        else if (info->id == 1) 
            ingredients = "Bread and Lettuce";
        else                    
            ingredients = "Bread and Cheese";

        pthread_mutex_lock(&tableMutex);
        printf("%s picks up %s\n", info->name, ingredients);
        printf("%s is making the sandwich...\n", info->name);
        printf("%s finished making the sandwich and eats it\n", info->name);
        printf("%s signals Supplier\n\n", info->name);
        pthread_mutex_unlock(&tableMutex);

        sem_post(&supplierSem);
    }
    return NULL;
}

int main() {
    srand((unsigned)time(NULL));

    printf("Enter the number of times the supplier places ingredients:\n");
    scanf("%d", &rounds);

    sem_init(&supplierSem, 0, 0);
    for (int i = 0; i < 3; i++)
        sem_init(&makerSem[i], 0, 0);

    MakerInfo makers[3] = {
        {0, "Maker A"},
        {1, "Maker B"},
        {2, "Maker C"}
    };

    pthread_t makerThreads[3];
    for (int i = 0; i < 3; i++)
        pthread_create(&makerThreads[i], NULL, maker, &makers[i]);

    pthread_t supplierThread;
    pthread_create(&supplierThread, NULL, supplier, NULL);

    pthread_join(supplierThread, NULL);

    for (int i = 0; i < 3; i++)
        pthread_cancel(makerThreads[i]);
    for (int i = 0; i < 3; i++)
        pthread_join(makerThreads[i], NULL);

    sem_destroy(&supplierSem);
    for (int i = 0; i < 3; i++)
        sem_destroy(&makerSem[i]);
    pthread_mutex_destroy(&tableMutex);

    return 0;
}