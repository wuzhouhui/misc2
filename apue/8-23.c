#include "apue.h"
#include <sys/wait.h>

int main(void)
{
    int status;

    status = system("date");
    pr_exit(status);

    status = system("nosuchcommand");
    pr_exit(status);

    status = system("who; exit 44");
    pr_exit(status);

    exit(0);
}
