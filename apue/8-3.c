#include "apue.h"

int globvar = 6;

int main(void)
{
    int var;
    pid_t pid;

    var = 88;
    printf("before vfork\n");
    if((pid = vfork()) == 0) {
        globvar++;
        var++;
        _exit(0);
    }
    printf("pid = %ld, glob = %d, var = %d\n", (long)getpid(), globvar, var );
    exit(0);
}
