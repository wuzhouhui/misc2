/* EXTERN should be extern except in table.c */
#ifdef _TABLE
#undef EXTERN
#define EXTERN
#endif

/* Global variables. */
// 与当前进程对应的 struct mproc
EXTERN struct mproc *mp;	/* ptr to 'mproc' slot of current process */
// 'mproc' 表中被占用的表项
EXTERN int procs_in_use;	/* how many processes are marked as IN_USE */
EXTERN char monitor_params[128*sizeof(char *)];	/* boot monitor parameters */
EXTERN struct kinfo kinfo;			/* kernel information */

/* The parameters of the call are kept here. */
// 发送给 PM 的消息
EXTERN message m_in;		/* the incoming message itself is kept here. */
/* 主调进程的进程号 */
EXTERN int who;			/* caller's proc number */
/* 系统调用号 */
EXTERN int call_nr;		/* system call number */

extern _PROTOTYPE (int (*call_vec[]), (void) );	/* system call handlers */
extern char core_name[];	/* file name where core images are produced */
EXTERN sigset_t core_sset;	/* which signals cause core images */
EXTERN sigset_t ign_sset;	/* which signals are by default ignored */

