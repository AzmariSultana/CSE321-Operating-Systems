#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main() {
    pid_t child_pid= fork();

    if (child_pid >0){
        printf("1. Parent process ID: %d\n", getpid());
        wait(NULL);
    }
    else if (child_pid==0){
        printf("2.Child process id: %d\n", getpid());

        for (int i=1; i<=3; i++){
            pid_t gc_pid=fork();

            if (gc_pid==0){
                printf("%d. Grand child process id: %d\n", i+2,getpid());
                return 0;
            }
        }
        for (int i=0; i<3;i++){
            wait(NULL);
        }
    }
    else{
        perror("fork failed");
    }
    return 0;
}