#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(void)
{
   /* pid_t fid = fork();

    if (fid == 0) {
        printf("I am the first child %d and my parent is %d\n", getpid(), getppid());
    } else {
        pid_t sid;

        printf("I am the parent %d and my parent is %d\n", getpid(), getppid());

        sid = fork();

        if (sid == 0) {
            printf("I am the second child %d and my parent is %d\n", getpid(), getppid());
        } else {
            printf("I am the parent %d and my parent is %d\n", getpid(), getppid());
        }
    } */

    int i;
    for (i = 0; i < 3; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            printf("child %d, pid %d, parent %d\n", i, getpid(), getppid());
            _exit(0);          /* child must NOT continue the loop */
        }
    }
    printf("parent %d made 3 children\n", getpid());

    return 0;
}
