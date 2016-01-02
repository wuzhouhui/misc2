#include "apue.h"
#include <sys/wait.h>

static void sig_int(int); //静态函数, 该函数的作用域仅限于本文件内

int main()
{
	char buf[MAXLINE];
	pid_t pid;
	int status;

	if(signal(SIGINT, sig_int)==SIG_ERR)
		err_sys("signal error");

	printf("%% ");
	while(fgets(buf, MAXLINE, stdin)!=NULL){
		if(buf[strlen(buf)-1]=='\n')
			buf[strlen(buf)-1]='\0';
		if((pid=fork())<0){
			//err_sys("fork error");
			;
		}else if(pid==0){
			execlp(buf, buf, (char *)0);
			//err_ret("couldn't execute: %s", buf);
			exit(127);
		}
		if((pid==waitpid(pid, &status, 0))<0)
			//err_sys("waitpid error");
			;
		printf("%% ");
	}
}

void sig_int(int signal){
	printf("interrupt\n%% ");
}
