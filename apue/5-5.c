#include "apue.h"
#define MAXLINE 4

int main(){
    char buf[MAXLINE];
    char afterbuf='a';
    fgets(buf, 9, stdin);
    fputs(buf, stdout);
    putchar(afterbuf);
    exit(0);
}
