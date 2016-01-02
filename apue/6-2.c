#include "apue.h"
#include <stddef.h>
#include <string.h>
struct passwd *getpwnam(const char *name)
{
    struct passwd *ptr=NULL;
    setpwent();
    while ((ptr = getpwent()) != NULL)
        if(strcmp(ptr->pw_name, name) == 0)
            break;
    endpwent();
    return ptr;
}
