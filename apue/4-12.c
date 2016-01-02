#include "apue.h"
int main(){
    struct stat statbuf;
    stat("foo", &statbuf);
    chmod("foo", (statbuf.st_mode&~S_IXGRP)|S_ISGID);
    chmod("bar", S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
    exit(0);
}
