#ifndef PRIV_H
#define PRIV_H

/* Declaration of the system privileges structure. It defines flags, system 
 * call masks, an synchronous alarm timer, I/O privileges, pending hardware 
 * interrupts and notifications, and so on.
 * System processes each get their own structure with properties, whereas all 
 * user processes share one structure. This setup provides a clear separation
 * between common and privileged process fields and is very space efficient. 
 *
 * Changes:
 *   Jul 01, 2005	Created.  (Jorrit N. Herder)	
 */
#include <minix/com.h>
#include "protect.h"
#include "const.h"
#include "type.h"
 
// 特权级结构
struct priv {
	// 与该特权级关联的进程号
  proc_nr_t s_proc_nr;		/* number of associated process */
  // 该结构在系统内的索引号
  sys_id_t s_id;		/* index of this system structure */
  // 标志
  short s_flags;		/* PREEMTIBLE, BILLABLE, etc. */

  // 允许的系统调用陷井
  short s_trap_mask;		/* allowed system call traps */
  // 指示可以从哪些进程接收消息
  sys_map_t s_ipc_from;		/* allowed callers to receive from */
  // 指示允许向哪些进程发送消息
  sys_map_t s_ipc_to;		/* allowed destination processes */
  // 允许的系统调用, 每一位代表一个系统调用.
  long s_call_mask;		/* allowed kernel calls */

  // 未决的通知位图, 每个系统进程的通知对应一位
  sys_map_t s_notify_pending;  	/* bit map with pending notifications */
  // 未决硬件中断位图
  irq_id_t s_int_pending;	/* pending hardware interrupts */
  // 未决信号位图
  sigset_t s_sig_pending;	/* pending signals */

  /* 同步闹钟定时器 */
  timer_t s_alarm_timer;	/* synchronous alarm timer */ 
  // 远程内存映射.
  struct far_mem s_farmem[NR_REMOTE_SEGS];  /* remote memory map */
  // 指向内核任务栈的警戒字的指针.
  reg_t *s_stack_guard;		/* stack guard word for kernel tasks */
};

/* Guard word for task stacks. */
/* 任务栈的警戒字 */
// 如果寄存器的的长度是16位, 将栈的警戒字设置为 0xBEEF, 否则 0xDEADBEEF.
#define STACK_GUARD	((reg_t) (sizeof(reg_t) == 2 ? 0xBEEF : 0xDEADBEEF))

/* Bits for the system property flags. */
#define PREEMPTIBLE	0x01	/* kernel tasks are not preemptible */ /* 内核任务不可抢占 */
#define BILLABLE	0x04	/* some processes are not billable */ /* 有些进程是不可记帐的 */
#define SYS_PROC	0x10	/* system processes are privileged */ /* 带特权的系统进程 */
// 正在调用 sendrec()
#define SENDREC_BUSY	0x20	/* sendrec() in progress */

/* Magic system structure table addresses. */
// 获取系统优先级表的地址
#define BEG_PRIV_ADDR (&priv[0])
// 获取系统优先级表的结束地址
#define END_PRIV_ADDR (&priv[NR_SYS_PROCS])

#define priv_addr(i)      (ppriv_addr)[(i)]
#define priv_id(rp)	  ((rp)->p_priv->s_id)
#define priv(rp)	  ((rp)->p_priv)

#define id_to_nr(id)	priv_addr(id)->s_proc_nr
// 由进程号得到该进程在系统内的索引号
#define nr_to_id(nr)    priv(proc_addr(nr))->s_id

/* The system structures table and pointers to individual table slots. The 
 * pointers allow faster access because now a process entry can be found by 
 * indexing the psys_addr array, while accessing an element i requires a 
 * multiplication with sizeof(struct sys) to determine the address. 
 */
/*
 * 系统特权级表与特权级指针表. 指针的访问速度更快, 因为某个进程的特权级
 * 可以通过索引物理地址数组找到, 而地址可以通过下标乘以 sizeof(struct sys)
 * 一得到.
 */
// 系统特权级表
EXTERN struct priv priv[NR_SYS_PROCS];		/* system properties table */
// 系统优先级指针表
EXTERN struct priv *ppriv_addr[NR_SYS_PROCS];	/* direct slot pointers */

/* Unprivileged user processes all share the same privilege structure.
 * This id must be fixed because it is used to check send mask entries.
 */
/*
 * 非特权的用户进程共享一个特权级结构体. 这个标识符必须是固定的,
 * 因为它会用来检查发送掩码入口.
 */
#define USER_PRIV_ID	0

/* Make sure the system can boot. The following sanity check verifies that
 * the system privileges table is large enough for the number of processes
 * in the boot image. 
 */
#if (NR_BOOT_PROCS > NR_SYS_PROCS)
#error NR_SYS_PROCS must be larger than NR_BOOT_PROCS
#endif

#endif /* PRIV_H */
