/* This table has one slot per process.  It contains all the process management
 * information for each process.  Among other things, it defines the text, data
 * and stack segments, uids and gids, and various flags.  The kernel and file
 * systems have tables that are also indexed by process, with the contents
 * of corresponding slots referring to the same process in all three.
 */
/*
 * 每一个进程都对应表中的一个槽. 这张表包含了所有的进程管理信息. 另外, 这个 
 * 文件还定义了文本段, 数据段与堆栈段, uids, gids 和其他标志. 内核与文件系
 * 统也有通过进程索引的表格, 与表槽相对应的内容都指向了同一个进程.
 */
#include <timers.h>

// management of process, 相当于 linux 的 struct task_struct
EXTERN struct mproc {
  // 局部段: 文本段, 数据段, 堆栈段 
  struct mem_map mp_seg[NR_LOCAL_SEGS]; /* points to text, data, stack */
  // 进程退出状态
  char mp_exitstatus;		/* storage for status when process exits */
  char mp_sigstatus;		/* storage for signal # for killed procs */
  // pid
  pid_t mp_pid;			/* process id */
  // gid
  pid_t mp_procgrp;		/* pid of process group (used for signals) */
  // 该进程在等待的进程号
  pid_t mp_wpid;		/* pid this process is waiting for */
  // parent pid
  int mp_parent;		/* index of parent process */

  /* Child user and system times. Accounting done on child exit. */
  // 子进程用户态的运行时间
  clock_t mp_child_utime;	/* cumulative user time of children */
  // 子进程内核态的运行时间
  clock_t mp_child_stime;	/* cumulative sys time of children */

  /* Real and effective uids and gids. */
  // 实际用户 ID 
  uid_t mp_realuid;		/* process' real uid */
  // 有效用户 ID 
  uid_t mp_effuid;		/* process' effective uid */
  // 实际组 ID 
  gid_t mp_realgid;		/* process' real gid */
  // 有效组 ID 
  gid_t mp_effgid;		/* process' effective gid */

  /* File identification for sharing. */
  // 执行文件的 inode 号, 如果调用了 exec()
  ino_t mp_ino;			/* inode number of file */
  // 文件系统的设备号 ???
  dev_t mp_dev;			/* device number of file system */
  time_t mp_ctime;		/* inode changed time */

  /* Signal handling information. */
  sigset_t mp_ignore;		/* 1 means ignore the signal, 0 means don't */
  sigset_t mp_catch;		/* 1 means catch the signal, 0 means don't */
  sigset_t mp_sig2mess;		/* 1 means transform into notify message */
  sigset_t mp_sigmask;		/* signals to be blocked */
  sigset_t mp_sigmask2;		/* saved copy of mp_sigmask */
  sigset_t mp_sigpending;	/* pending signals to be handled */
  struct sigaction mp_sigact[_NSIG + 1]; /* as in sigaction(2) */
  vir_bytes mp_sigreturn; 	/* address of C library __sigreturn function */
  struct timer mp_timer;	/* watchdog timer for alarm(2) */

  /* Backwards compatibility for signals. */
  sighandler_t mp_func;		/* all sigs vectored to a single user fcn */

  unsigned mp_flags;		/* flag bits */
  vir_bytes mp_procargs;        /* ptr to proc's initial stack arguments */
  // 等待换入的等待进程
  struct mproc *mp_swapq;	/* queue of procs waiting to be swapped in */
  // 将要发送给某个进程的回复消息
  message mp_reply;		/* reply message to be sent to one */

  /* Scheduling priority. */
  signed int mp_nice;		/* nice is PRIO_MIN..PRIO_MAX, standard 0. */

  char mp_name[PROC_NAME_LEN];	/* process name */
} mproc[NR_PROCS];

/* Flag values */
#define IN_USE          0x001	/* set when 'mproc' slot in use */
#define WAITING         0x002	/* set by WAIT system call */
#define ZOMBIE          0x004	/* set by EXIT, cleared by WAIT */
#define PAUSED          0x008	/* set by PAUSE system call */
#define ALARM_ON        0x010	/* set when SIGALRM timer started */
// 如果指令空间与数据空是分开的, 则置位
#define SEPARATE	0x020	/* set if file is separate I & D space */
#define	TRACED		0x040	/* set if process is to be traced */
#define STOPPED		0x080	/* set if process stopped for tracing */
#define SIGSUSPENDED 	0x100	/* set by SIGSUSPEND system call */
#define REPLY	 	0x200	/* set if a reply message is pending */
#define ONSWAP	 	0x400	/* set if data segment is swapped out */
#define SWAPIN	 	0x800	/* set if on the "swap this in" queue */
#define DONT_SWAP      0x1000   /* never swap out this process */
#define PRIV_PROC      0x2000   /* system process, special privileges */

#define NIL_MPROC ((struct mproc *) 0)

