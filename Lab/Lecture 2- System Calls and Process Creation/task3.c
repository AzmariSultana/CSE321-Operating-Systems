#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main() {
    pid_t a,b,c;
    int count=1;

    a=fork();
    b=fork();
    c=fork();

    if (getpid() % 2!=0) {
        pid_t extra= fork();
        if (extra==0) {
            count=1;
        }
    }

    sleep(1);

    if (getppid() !=1) {
        while (wait(NULL)>0);
    }
    printf("Process PID: %d\n",getpid());

    return 0;
}