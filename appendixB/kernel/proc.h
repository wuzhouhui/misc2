#ifndef PROC_H
#define PROC_H

/* Here is the declaration of the process table.  It contains all process
 * data, including registers, flags, scheduling priority, memory map, 
 * accounting, message passing (IPC) information, and so on. 
 *
 * Many assembly code routines reference fields in it.  The offsets to these
 * fields are defined in the assembler include file sconst.h.  When changing
 * struct proc, be sure to change sconst.h to match.
 */
#include <minix/com.h>
#include "protect.h"
#include "const.h"
#include "priv.h"
 
// 进程结构体
struct proc {
// 在栈中保存的进程寄存器
  struct stackframe_s p_reg;	/* process' registers saved in stack frame */
  // 进程的局部描述符表对应的选择符, 在 GDT 中选择.
  reg_t p_ldt_sel;		/* selector in gdt with ldt base and limit */
  // 局部描述符表
  struct segdesc_s p_ldt[2+NR_REMOTE_SEGS]; /* CS, DS and remote segments */

  // 进程号.
  proc_nr_t p_nr;		/* number of this process (for fast access) */
  struct priv *p_priv;		/* system privileges structure */
  // 与消息, 信号 发送接收有关的阻塞标志
  char p_rts_flags;		/* SENDING, RECEIVING, etc. */

  // 调度优先级, 值越小, 优先级越高
  char p_priority;		/* current scheduling priority */
  // 进程所能拥有的最高的优先级
  char p_max_priority;		/* maximum scheduling priority */
  // 剩余的调度时间片
  char p_ticks_left;		/* number of scheduling ticks left */
  // 时间片原子值, 在分配时间片时, 分配值须是该值的整数倍.
  char p_quantum_size;		/* quantum size in ticks */

  // 代码码, 数据段, 堆栈段的内存映射
  struct mem_map p_memmap[NR_LOCAL_SEGS];   /* memory map (T, D, S) */

  // 进程的用户态运行时间, 单位 滴答
  clock_t p_user_time;		/* user time in ticks */
  // 进程的用户态运行时间, 单位 滴答
  clock_t p_sys_time;		/* sys time in ticks */

  // 指向下一个就绪的进程
  struct proc *p_nextready;	/* pointer to next ready process */
  // 想要向该进程发送消息的进程链表
  struct proc *p_caller_q;	/* head of list of procs wishing to send */
  // 指向链表中的下一个元素.
  struct proc *p_q_link;	/* link to next proc wishing to send */
  // 被递送的消息缓冲区指针
  message *p_messbuf;		/* pointer to passed message buffer */
  // 该进程想接收哪个进程发送的消息
  proc_nr_t p_getfrom;		/* from whom does process want to receive? */
  // 该进程想向哪个进程发送信号
  proc_nr_t p_sendto;		/* to whom does process want to send? */

  // 未决的内核信号
  sigset_t p_pending;		/* bit map for pending kernel signals */

  // 进程名
  char p_name[P_NAME_LEN];	/* name of the process, including \0 */
};

/* Bits for the runtime flags. A process is runnable iff p_rts_flags == 0. */
#define SLOT_FREE	0x01	/* process slot is free */
// 阻止未映射的, 派生的子进程运行.
#define NO_MAP		0x02	/* keeps unmapped forked child from running */
// 因发送消息而阻塞
#define SENDING		0x04	/* process blocked trying to SEND */
// 因接收消息而阻塞
#define RECEIVING	0x08	/* process blocked trying to RECEIVE */
// 有新的内核信号到达
#define SIGNALED	0x10	/* set when new kernel signal arrives */
// 未就绪, 因为正在处理信号
#define SIG_PENDING	0x20	/* unready while signal being processed */
// 当进程被追踪时设置
#define P_STOP		0x40	/* set when process is being traced */
// 阻止派生的系统进程运行
#define NO_PRIV		0x80	/* keep forked system process from running */

/* Scheduling priorities for p_priority. Values must start at zero (highest
 * priority) and increment.  Priorities of the processes in the boot image 
 * can be set in table.c. IDLE must have a queue for itself, to prevent low 
 * priority user processes to run round-robin with IDLE. 
 */
// 调度队列的个数, 比最低优先级的值大1, 因为优先级从0开始
#define NR_SCHED_QUEUES   16	/* MUST equal minimum priority + 1 */
#define TASK_Q		   0	/* highest, used for kernel tasks */
#define MAX_USER_Q  	   0    /* highest priority for user processes */   
#define USER_Q  	   7    /* default (should correspond to nice 0) */   
#define MIN_USER_Q	  14	/* minimum priority for user processes */
// 空闲进程的优先级, 系统内最低的优先级
#define IDLE_Q		  15    /* lowest, only IDLE process goes here */

/* Magic process table addresses. */
// 进程表地址.
#define BEG_PROC_ADDR (&proc[0])
// 用户进程表地址
#define BEG_USER_ADDR (&proc[NR_TASKS])
#define END_PROC_ADDR (&proc[NR_TASKS + NR_PROCS])

// NULL process
#define NIL_PROC          ((struct proc *) 0)		
#define NIL_SYS_PROC      ((struct proc *) 1)		
// 进程号为 n 的进程地址, proc 数组前 NR_TASKS 留给任务
// (任务?? 进程??), 宏 cproc_addr 与 proc_addr 有何不同??,
// 它们的功能是一样的, 都是根据进程号获取进程指针.
#define cproc_addr(n)     (&(proc + NR_TASKS)[(n)])
// 进程号为 n 的进程指针
#define proc_addr(n)      (pproc_addr + NR_TASKS)[(n)]
#define proc_nr(p) 	  ((p)->p_nr)

// 判断进程号是否有效.
#define isokprocn(n)      ((unsigned) ((n) + NR_TASKS) < NR_PROCS + NR_TASKS)
#define isemptyn(n)       isemptyp(proc_addr(n)) 
#define isemptyp(p)       ((p)->p_rts_flags == SLOT_FREE)
// 判断进程指针所指的进程是否是内核任务.
#define iskernelp(p)	  iskerneln((p)->p_nr)
// 判断进程号为 n 的进程是否是内核任务(进程), 如 CLOCK, HARDWARE
#define iskerneln(n)	  ((n) < 0)
#define isuserp(p)        isusern((p)->p_nr)
// 判断进程号为 n 的进程是否是用户进程.
#define isusern(n)        ((n) >= 0)

/* The process table and pointers to process table slots. The pointers allow
 * faster access because now a process entry can be found by indexing the
 * pproc_addr array, while accessing an element i requires a multiplication
 * with sizeof(struct proc) to determine the address. 
 */
/*
 * 进程表与进程指针表. 通过指针访问更加迅速, 因为某个进程项可以通过索引
 * pproc_addr 数组找到, 只要将下标 i 乘上 sizeof(struct proc) 就可以确定
 * 元素的地址.
 */
// 进程数组
EXTERN struct proc proc[NR_TASKS + NR_PROCS];	/* process table */
// 进程指针数组
EXTERN struct proc *pproc_addr[NR_TASKS + NR_PROCS];
// 队列头指针数组
EXTERN struct proc *rdy_head[NR_SCHED_QUEUES]; /* ptrs to ready list headers */
// 队列尾指针数组
EXTERN struct proc *rdy_tail[NR_SCHED_QUEUES]; /* ptrs to ready list tails */

#endif /* PROC_H */
