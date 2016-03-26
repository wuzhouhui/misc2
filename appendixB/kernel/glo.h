#ifndef GLO_H
#define GLO_H

/* Global variables used in the kernel. This file contains the declarations;
 * storage space for the variables is allocated in table.c, because EXTERN is
 * defined as extern unless the _TABLE definition is seen. We rely on the 
 * compiler's default initialization (0) for several global variables. 
 */
#ifdef _TABLE
#undef EXTERN
#define EXTERN
#endif

#include <minix/config.h>
#include "config.h"

/* Variables relating to shutting down MINIX. */
EXTERN char kernel_exception;		/* TRUE after system exceptions */
EXTERN char shutdown_started;		/* TRUE after shutdowns / reboots */

/* Kernel information structures. This groups vital kernel information. */
/* 内核信息结构. 这些结构组成了至关重要的内核信息 */
// a.out 头部数组的地址
EXTERN phys_bytes aout;			/* address of a.out headers */
EXTERN struct kinfo kinfo;		/* kernel information for users */
// 关于机器的信息
EXTERN struct machine machine;		/* machine information for users */
// 内核的诊断消息
EXTERN struct kmessages kmess;  	/* diagnostic messages in kernel */
/* 搜集内核的随机信息 */
EXTERN struct randomness krandom;	/* gather kernel random information */

/* Process scheduling information and the kernel reentry count. */
// 上一次运行的进程
EXTERN struct proc *prev_ptr;	/* previously running process */
// 当前进程, 相当于 Linux 中的 CURRENT
EXTERN struct proc *proc_ptr;	/* pointer to currently running process */
EXTERN struct proc *next_ptr;	/* next process to run after restart() */
// 为时钟滴答记帐的进程
EXTERN struct proc *bill_ptr;	/* process to bill for clock ticks */
// 内核重入计数, 每入一次递减 1
EXTERN char k_reenter;		/* kernel reentry count (entry count less 1) */
// 在时钟任务之外计数的时钟滴答数
EXTERN unsigned lost_ticks;	/* clock ticks counted outside clock task */

/* Interrupt related variables. */
// 中断请求钩子数组
EXTERN irq_hook_t irq_hooks[NR_IRQ_HOOKS];	/* hooks for general use */
// 中断请求钩子指针数组, 每一个 irq 对应数组中的一个元素,
// 数组中的元素后跟多个中断处理程序, 这些中断处理程序使用链表相互连接,
// 因此称为 "钩子". 可见一个中断请求会调用多个中断处理程序.
EXTERN irq_hook_t *irq_handlers[NR_IRQ_VECTORS];/* list of IRQ handlers */
EXTERN int irq_actids[NR_IRQ_VECTORS];		/* IRQ ID bits active */
// 中断请求位图, 每一位代表一个中断请求号, 若该中断请求有对应的中断
// 处理函数, 则开启, 否则关闭.
EXTERN int irq_use;				/* map of all in-use irq's */

/* Miscellaneous. */
EXTERN reg_t mon_ss, mon_sp;		/* boot monitor stack */
/* 非零, 我们可以回到监视器 */
EXTERN int mon_return;			/* true if we can return to monitor */

/* Variables that are initialized elsewhere are just extern here. */
extern struct boot_image image[]; 	/* system image processes */
extern char *t_stack[];			/* task stack space */
extern struct segdesc_s gdt[];		/* global descriptor table */

EXTERN _PROTOTYPE( void (*level0_func), (void) );

#endif /* GLO_H */





