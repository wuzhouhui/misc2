#include "apue.h"
#include <fcntl.h>
int main(int argc, char *argv[]){
    if(access(argv[1], R_OK)<0)
        printf("access error for %s\n", argv[1]);
    else 
        printf("read access OK\n");
    if(open(argv[1], O_RDONLY)<0)
        printf("open for reading error\n");
    else 
        printf("open for reading OK\n");
    exit(0);
}
