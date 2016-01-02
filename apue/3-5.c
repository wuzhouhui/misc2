#include "apue.h"
#define BUFFSIZE 1024
int main(){
    int n;
    char buf[BUFFSIZE];
    n=read(STDIN_FILENO, buf, BUFFSIZE);
    write(STDOUT_FILENO, buf, n);
    exit(0);
}
