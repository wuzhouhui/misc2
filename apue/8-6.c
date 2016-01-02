#include <sys/wait.h>

extern void pr_exit(int);

int main()
{
    pid_t pid;
    int status;

    if ((pid = fork()) == 0)
        exit(7);

    wait(&status);
    pr_exit(status);

    if ((pid = fork()) == 0)
        abort();

    wait(&status);
    pr_exit(status);

    if ((pid = fork()) == 0)
        status /= 0;
    wait(&status);
    pr_exit(status);

    exit(0);
}
