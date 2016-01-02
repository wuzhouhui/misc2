#include "apue.h"
#include <time.h>
int main()
{
    time_t caltime;
    struct tm *tm;
    char line[MAXLINE];
    if ((caltime = time(NULL)) == -1)
        printf("time error\n");
    else if ((tm = localtime(&caltime)) == NULL)
        printf("localtime error\n");
    else if (strftime(line, MAXLINE, "%a %b %d %X %Z %Y\n", tm) == 0)
        printf("strftime error\n");
    else
        fputs(line, stdout);
    exit(0);
}
