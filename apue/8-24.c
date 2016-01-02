#include "apue.h"
 
int main(int argc, char *argv[])
{
    int status;

    status = system(argv[1]);
    pr_exit(status);
    exit(0);
}
