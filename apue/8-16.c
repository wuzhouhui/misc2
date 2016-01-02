#include "apue.h"
#include <sys/wait.h>

char *env_init[] = {"USER=unknown", "PATH=/tmp", NULL};

int main (void)
{
    pid_t pid;

    pid = fork();
    if (pid  == 0) {
        execle("/home/dell/Documents/apue/echoall", "echoall", "myarg1", "MY ARG2", (char *)0, env_init);
    }
    waitpid(pid, NULL, 0);
    pid = fork();
    if (pid == 0) {
        execlp("/home/dell/Documents/apue/echoall", "echoall", "only 1 arg", (char *)0);
    }
    exit(0);
}
