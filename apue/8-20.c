#include "apue.h"
#include <sys/wait.h>

int main(void)
{
    pid_t pid;

    pid = fork();
    if (pid == 0) {
        execl("/home/dell/Documents/apue/testinterp", 
                "testinterp", "myarg1", "MY ARG2", (char *)0);
    }
    waitpid(pid, NULL, 0);
    exit(0);
}
