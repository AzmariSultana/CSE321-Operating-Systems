#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main (int argc, char *argv[]) {
    int n= argc-1;
    int arr[n];

    for (int i=0; i<n; i++){
        arr[i]= atoi(argv[i+1]);
    }

    pid_t pid=fork();

    if (pid==0) {

        for (int i=0; i<n-1;i++){
            for (int j=i+1; j<n;j++){
                if (arr[i] < arr[j]) {
                    int temp= arr[i];
                    arr[i]=arr[j];
                    arr[j]=temp;
                }
            }
        }
        printf("Child Process: Sorted (Descending): ");
        for (int i=0; i<n; i++) {
            printf("%d", arr[i]);
        }
        printf("\n");
    }
    else if (pid>0){
        wait(NULL);
        printf("Parent process: Odd/Even Status:\n");

        for (int i=0; i<n; i++){
            if (arr[i]%2==0)
                printf("%d is Even\n", arr[i]);
            else
                printf("%d is odd\n", arr[i]);
        }
    }
    else {
        perror("fork failed");
    }
    return 0;
}