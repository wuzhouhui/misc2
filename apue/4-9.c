#include "apue.h"
#include <fcntl.h>
#define RWRWRW (S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH)
int main(){
    umask(0);
    creat("foo", RWRWRW);
    umask(S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH);
    creat("bar", RWRWRW);
    exit(0);
}
