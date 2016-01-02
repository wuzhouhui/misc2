#include "apue.h"
char buf1[]="abcdefghij";
char buf2[]="ABCDEFGHIJ";
int main(){
    int fd;
    if((fd=creat("file.hole", FILE_MODE))<0)
        printf("create error\n");
    else {
        if(write(fd, buf1, 10)!=10)
            printf("buf1 write error\n");
        else {
            lseek(fd, 16384, SEEK_SET);
            write(fd, buf2, 10);
            exit(0);
        }
    }
}
