#include "apue.h"
#include <shadow.h>
int main()
{
    struct spwd *ptr;
    if ((ptr = getspnam("dell")) == NULL)
        printf("getspam error\n");
    else
        printf("sp_pwdp = %s\n", ptr->sp_pwdp == NULL || ptr->sp_pwdp[0] == 0 ? "(null)" : ptr->sp_pwdp);
    exit(0);
}

