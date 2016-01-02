#include "apue.h"
#include <fcntl.h>

int main(int argc, char *argv[]){
    int i, fd;
    struct stat statbuf;
    struct timespec times[2];
    for(i=1; i<argc; i++){
        stat(argv[i], &statbuf);
        fd=open(argv[i], O_RDWR|O_TRUNC);
        times[0]=statbuf.st_atim;
        times[1]=statbuf.st_mtim;
        futimens(fd, times);
        close(fd);
    }
    exit(0);
}
