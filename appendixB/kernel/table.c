/* The object file of "table.c" contains most kernel data. Variables that 
 * are declared in the *.h files appear with EXTERN in front of them, as in
 *
 *    EXTERN int x;
 *
 * Normally EXTERN is defined as extern, so when they are included in another
 * file, no storage is allocated.  If EXTERN were not present, but just say,
 *
 *    int x;
 *
 * then including this file in several source files would cause 'x' to be
 * declared several times.  While some linkers accept this, others do not,
 * so they are declared extern when included normally.  However, it must be
 * declared for real somewhere.  That is done here, by redefining EXTERN as
 * the null string, so that inclusion of all *.h files in table.c actually
 * generates storage for them.  
 *
 * Various variables could not be declared EXTERN, but are declared PUBLIC
 * or PRIVATE. The reason for this is that extern variables cannot have a  
 * default initialization. If such variables are shared, they must also be
 * declared in one of the *.h files without the initialization.  Examples 
 * include 'boot_image' (this file) and 'idt' and 'gdt' (protect.c). 
 *
 * Changes:
 *    Aug 02, 2005   set privileges and minimal boot image (Jorrit N. Herder)
 *    Oct 17, 2004   updated above and tasktab comments  (Jorrit N. Herder)
 *    May 01, 2004   changed struct for system image  (Jorrit N. Herder)
 */
/*
 * "table.c" 的目标文件包含内核大部分的数据. 在 *.h 文件中声明的以 EXTERN 
 * 开始的变量, 例如:
 *	EXTERN int x;
 * 通常来说 EXTERN 被定义成 extern, 所有当这些语句包含在其他文件中时,
 * 并没有分本存储空间, 如果 EXTERN 没有出现, 只写了:
 *	int	x;
 * 包含这个文件的源文件将会使得 'x' 被声明多次. 有一些链接器会接受这种
 * 情况, 另一些则不然, 所有它们被包含时总是被声明成 extern. 但是, 这些 
 * 变量必须在某处定义. 定义在这个文件中完成, 只要将 EXTERN 重新定义成空
 * 字符串即可, 所以 table.c 为所有包含的 *.h 文件中声明的全书变量都分配
 * 了存储空间.
 *
 * 许多变量不能声明成 EXTERN, 而是声明成 PUBLIC 或 PRIVATE. 原因是外部
 * 变量没有默认的初始值. 如果这样的变量被共享, 它必须在某一个头文件中
 * 声明, 且没有初始化. 这样的例子包括 'boot_image'(本文件) 和 'idt',
 * 'gdt'(protect.c).
 *
 * 更改:
 *	2005/08/02  设置特权级与最小化的引导映像;
 *	2004/08/17  ...
 *	2004/05/01  ...
 */
#define _TABLE

#include "kernel.h"
#include "proc.h"
#include "ipc.h"
#include <minix/com.h>
#include <ibm/int86.h>

/* Define stack sizes for the kernel tasks included in the system image. */
/* 定义内核任务的栈大小, 这些内核任务包含在系统映像中 */
#define NO_STACK	0
#define SMALL_STACK	(128 * sizeof(char *)) // 小型栈
#define IDL_S	SMALL_STACK	/* 3 intr, 3 temps, 4 db for Intel */
// 哑任务, 使用内核栈
#define	HRD_S	NO_STACK	/* dummy task, uses kernel stack */
// SYSTEM 和 CLOCK 任务.
#define	TSK_S	SMALL_STACK	/* system and clock task */

/* Stack space for all the task stacks.  Declared as (char *) to align it. */
// 所有任务栈的栈空间. 这个栈空间被任务分段共享.
#define	TOT_STACK_SPACE	(IDL_S + HRD_S + (2 * TSK_S))
PUBLIC char *t_stack[TOT_STACK_SPACE / sizeof(char *)];
	
/* Define flags for the various process types. */
#define IDL_F 	(SYS_PROC | PREEMPTIBLE | BILLABLE)	/* idle task */ /* 空闲任务标志: 带特权的系统进程, 可抢占, 可记账 */
#define TSK_F 	(SYS_PROC)				/* kernel tasks */ /* 内核任务标志: 带特权的系统进程. */
#define SRV_F 	(SYS_PROC | PREEMPTIBLE)		/* system services */ /* 系统服务标志: 带特权的系统进程, 可抢占. */
#define USR_F	(BILLABLE | PREEMPTIBLE)		/* user processes */ /* 用户进程标志: 可记账, 可抢点. */

/* Define system call traps for the various process types. These call masks
 * determine what system call traps a process is allowed to make.
 */
/*
 * 为不同的进程类型定义系统调用陷阱. 这些调用掩码决定了一个进程可以调用
 * 哪些调用调用.
 */
#define TSK_T	(1 << RECEIVE)			/* clock and system */
#define SRV_T	(~0)				/* system services */
#define USR_T   ((1 << SENDREC) | (1 << ECHO))	/* user processes */

/* Send masks determine to whom processes can send messages or notifications. 
 * The values here are used for the processes in the boot image. We rely on 
 * the initialization code in main() to match the s_nr_to_id() mapping for the
 * processes in the boot image, so that the send mask that is defined here 
 * can be directly copied onto map[0] of the actual send mask. Privilege
 * structure 0 is shared by user processes. 
 */
/*
 * 发送掩码决定了进程可以向谁发送消息或通知. 这里定义的值会被引导映像中的
 * 程序使用. 我们依赖 main() 函数中的初始化代码, 初始化代码会为引导映像中
 * 的程序匹配 s_nr_to_id() 映射关系, 所有发送掩码定义在这里可以直接复制到
 * 真正的发送掩码 map[0] 中. 特权级结构 0 被所有的用户进程共享.
 */
#define s(n)		(1 << s_nr_to_id(n))
#define SRV_M (~0)
#define SYS_M (~0)
#define USR_M (s(PM_PROC_NR) | s(FS_PROC_NR) | s(RS_PROC_NR))
#define DRV_M (USR_M | s(SYSTEM) | s(CLOCK) | s(LOG_PROC_NR) | s(TTY_PROC_NR))

/* Define kernel calls that processes are allowed to make. This is not looking
 * very nice, but we need to define the access rights on a per call basis. 
 * Note that the reincarnation server has all bits on, because it should
 * be allowed to distribute rights to services that it starts. 
 */
/*
 * 定义进程允许的系统调用. 虽然看起来并不是很好, 但是我们需要为每一个调用
 * 定义访问权限. 注意, 再生进程开启所有的二进制位, 因为它需要向由它启动的
 * 服务分配权限.
 */
// 获取系统调用编号, 从 0 开始.
#define c(n)	(1 << ((n)-KERNEL_CALL))
// 再生进程开启所有的二进制位
#define RS_C	~0	
#define PM_C	~(c(SYS_DEVIO) | c(SYS_SDEVIO) | c(SYS_VDEVIO) \
    | c(SYS_IRQCTL) | c(SYS_INT86))
#define FS_C	(c(SYS_KILL) | c(SYS_VIRCOPY) | c(SYS_VIRVCOPY) | c(SYS_UMAP) \
    | c(SYS_GETINFO) | c(SYS_EXIT) | c(SYS_TIMES) | c(SYS_SETALARM))
#define DRV_C	(FS_C | c(SYS_SEGCTL) | c(SYS_IRQCTL) | c(SYS_INT86) \
    | c(SYS_DEVIO) | c(SYS_VDEVIO) | c(SYS_SDEVIO)) 
#define MEM_C	(DRV_C | c(SYS_PHYSCOPY) | c(SYS_PHYSVCOPY))

/* The system image table lists all programs that are part of the boot image. 
 * The order of the entries here MUST agree with the order of the programs
 * in the boot image and all kernel tasks must come first.
 * Each entry provides the process number, flags, quantum size (qs), scheduling
 * queue, allowed traps, ipc mask, and a name for the process table. The 
 * initial program counter and stack size is also provided for kernel tasks.
 */
/*
 * 系统映像表列出了包含在引导映像中的所有程序. 表中项目的顺序必须与程序在
 * 引导映像中的顺序完全一致, 并且所有的内核任务排在前面.
 * 表格中的每一项都包括了进程号, 标记, 时间片原子值, 调度队列, 允许的陷阱,
 * ipc 掩码, 以及提供给进程表的进程名. 初始程序的时间片与栈大小也提供给
 * 了内核任务.
 */
PUBLIC struct boot_image image[] = {
	[0] = {
		.proc_nr	= IDLE,
		.initial_pc	= idle_task,
		.flags		= IDL_F,
		.quantum	= 8,
		.priority	= IDLE_Q,
		.stksize	= IDL_S,
		.trap_mask	= 0,
		.ipc_to		= 0,
		.call_mask	= 0,
		.proc_name	= "IDLE",
	},
	[1] = {
		.proc_nr	= CLOCK,
		.initial_pc	= clock_task,
		.flags		= TSK_F,
		.quantum	= 64,
		.priority	= TASK_Q,
		.stksize	= TSK_S,
		.trap_mask	= TSK_T,
		.ipc_to		= 0,
		.call_mask	= 0,
		.proc_name	= "CLOCK",
	},
	[2] = {
		.proc_nr	= SYSTEM,
		.initial_pc	= sys_task,
		.flags		= TSK_F,
		.quantum	= 64,
		.priority	= TASK_Q,
		.stksize	= TSK_S,
		.trap_mask	= TSK_T,
		.ipc_to		= 0,
		.call_mask	= 0,
		.proc_name	= "SYSTEM",
	},
	[3] = {
		.proc_nr	= HARDWARE,
		.initial_pc	= 0,
		.flags		= TSK_F,
		.quantum	= 64,
		.priority	= TASK_Q,
		.stksize	= HRD_S,
		.trap_mask	= 0,
		.ipc_to		= 0,
		.call_mask	= 0,
		.proc_name	= "KERNEL",
	},
	[4] = {
		.proc_nr	= PM_PROC_NR,
		.initial_pc	= 0,
		.flags		= SRV_F,
		.quantum	= 32,
		.priority	= 3,
		.stksize	= 0,
		.trap_mask	= SRV_T,
		.ipc_to		= SRV_M,
		.call_mask	= PM_C,
		.proc_name	= "pm",
	},
	[5] = {
		.proc_nr	= FS_PROC_NR,
		.initial_pc	= 0,
		.flags		= SRV_F,
		.quantum	= 32,
		.priority	= 4,
		.stksize	= 0,
		.trap_mask	= SRV_T,
		.ipc_to		= SRV_M,
		.call_mask	= FS_C,
		.proc_name	= "fs",
	},
	[6] = {
		.proc_nr	= RS_PROC_NR,
		.initial_pc	= 0,
		.flags		= SRV_F,
		.quantum	= 4,
		.priority	= 3,
		.stksize	= 0,
		.trap_mask	= SRV_T,
		.ipc_to		= SYS_M,
		.call_mask	= RS_C,
		.proc_name	= "rs",
	},
	[7] = {
		.proc_nr	= TTY_PROC_NR,
		.initial_pc	= 0,
		.flags		= SRV_F,
		.quantum	= 4,
		.priority	= 1,
		.stksize	= 0,
		.trap_mask	= SRV_T,
		.ipc_to		= SYS_M,
		.call_mask	= DRV_C,
		.proc_name	= "tty",
	},
	[8] = {
		.proc_nr	= MEM_PROC_NR,
		.initial_pc	= 0,
		.flags		= SRV_F,
		.quantum	= 4,
		.priority	= 2,
		.stksize	= 0,
		.trap_mask	= SRV_T,
		.ipc_to		= DRV_M,
		.call_mask	= MEM_C,
		.proc_name	= "memory",
	},
	[9] = {
		.proc_nr	= LOG_PROC_NR,
		.initial_pc	= 0,
		.flags		= SRV_F,
		.quantum	= 4,
		.priority	= 2,
		.stksize	= 0,
		.trap_mask	= SRV_T,
		.ipc_to		= SYS_M,
		.call_mask	= DRV_C,
		.proc_name	= "log",
	},
	[10] = {
		.proc_nr	= DRVR_PROC_NR,
		.initial_pc	= 0,
		.flags		= SRV_F,
		.quantum	= 4,
		.priority	= 2,
		.stksize	= 0,
		.trap_mask	= SRV_T,
		.ipc_to		= SYS_M,
		.call_mask	= DRV_C,
		.proc_name	= "driver",
	},
	[11] = {
		.proc_nr	= INIT_PROC_NR,
		.initial_pc	= 0,
		.flags		= USR_F,
		.quantum	= 8,
		.priority	= USER_Q,
		.stksize	= 0,
		.trap_mask	= USR_T,
		.ipc_to		= USR_M,
		.call_mask	= 0,
		.proc_name	= "init",
	},
};
//PUBLIC struct boot_image image[] = {
///* process nr,   pc, flags, qs,  queue, stack, traps, ipcto, call,  name */ 
///* 进程号, 开始函数, 任务标志, 时间片原子值, 调度优先级, 栈大小, 
// * 系统调用陷阱屏蔽码, 发送掩码保护, 系统调用屏蔽码, 进程名
// */
// { IDLE,  idle_task /* _idle_task */, IDL_F,  8, IDLE_Q, IDL_S,     0,     0,     0, "IDLE"  },
// { CLOCK,clock_task, TSK_F, 64, TASK_Q, TSK_S, TSK_T,     0,     0, "CLOCK" },
// { SYSTEM, sys_task, TSK_F, 64, TASK_Q, TSK_S, TSK_T,     0,     0, "SYSTEM"},
// { HARDWARE,      0, TSK_F, 64, TASK_Q, HRD_S,     0,     0,     0, "KERNEL"},
// { PM_PROC_NR,    0, SRV_F, 32,      3, 0,     SRV_T, SRV_M,  PM_C, "pm"    },
// { FS_PROC_NR,    0, SRV_F, 32,      4, 0,     SRV_T, SRV_M,  FS_C, "fs"    },
// { RS_PROC_NR,    0, SRV_F,  4,      3, 0,     SRV_T, SYS_M,  RS_C, "rs"    },
// { TTY_PROC_NR,   0, SRV_F,  4,      1, 0,     SRV_T, SYS_M, DRV_C, "tty"   },
// { MEM_PROC_NR,   0, SRV_F,  4,      2, 0,     SRV_T, DRV_M, MEM_C, "memory"},
// { LOG_PROC_NR,   0, SRV_F,  4,      2, 0,     SRV_T, SYS_M, DRV_C, "log"   },
// { DRVR_PROC_NR,  0, SRV_F,  4,      2, 0,     SRV_T, SYS_M, DRV_C, "driver"},
// { INIT_PROC_NR,  0, USR_F,  8, USER_Q, 0,     USR_T, USR_M,     0, "init"  },
//};

/* Verify the size of the system image table at compile time. Also verify that 
 * the first chunk of the ipc mask has enough bits to accommodate the processes
 * in the image.  
 * If a problem is detected, the size of the 'dummy' array will be negative, 
 * causing a compile time error. Note that no space is actually allocated 
 * because 'dummy' is declared extern.
 */
/*
 * 在编译时验证系统映像文件的大小. 同时也验证 ipc * 掩码的第一块位图拥有足够
 * 的二进制位来容纳映像文件中的进程.
 * 如果发生了一个问题, 'dummy' 数组的大小将会是负的, * 这会导致一个编译时错误.
 * 注意, 这里的声明并不会分配存储空间, 因为 'extern'.
 */
// 那么链接的时候不会报错吗???
extern int dummy[(NR_BOOT_PROCS==sizeof(image)/
	sizeof(struct boot_image))?1:-1];
extern int dummy[(BITCHUNK_BITS > NR_BOOT_PROCS - 1) ? 1 : -1];

