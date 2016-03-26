#ifndef TYPE_H
#define TYPE_H

typedef _PROTOTYPE( void task_t, (void) );

/* Process table and system property related types. */ 
typedef int proc_nr_t;			/* process table entry number */
typedef short sys_id_t;			/* system process index */
// 位图, 每个系统进程对应一个位
typedef struct {			/* bitmap for system indexes */
  bitchunk_t chunk[BITMAP_CHUNKS(NR_SYS_PROCS)];
} sys_map_t;

// 包含在系统引导映像文件中的程序及其它们的属性.
struct boot_image {
  // 进程号
  proc_nr_t proc_nr;			/* process number to use */
  // task_t ???
  // 任务的启动函数
  task_t *initial_pc;			/* start function for tasks */
  // 进程标志
  int flags;				/* process flags */
  unsigned char quantum;		/* quantum (tick count) */
  // 调度优先级
  int priority;				/* scheduling priority */
  // 栈大小 
  int stksize;				/* stack size for tasks */
  short trap_mask;			/* allowed system call traps */
  // 发送掩码
  bitchunk_t ipc_to;			/* send mask protection */
  long call_mask;			/* system call protection */
  // 进程在进程表中的名字
  char proc_name[P_NAME_LEN];		/* name in process table */
};

struct memory {
  phys_clicks base;			/* start address of chunk */
  phys_clicks size;			/* size of memory chunk */
};

/* The kernel outputs diagnostic messages in a circular buffer. */
/* 内核往一个环状缓冲区中输出诊断消息. */
struct kmessages {
// 下一个可写入的索引.
  int km_next;				/* next index to write */
  int km_size;				/* current size in buffer */
  char km_buf[KMESS_BUF_SIZE];		/* buffer for messages */
};

struct randomness {
  struct {
	int r_next;				/* next index to write */
	int r_size;				/* number of random elements */
	// 随机信息缓冲区
	unsigned short r_buf[RANDOM_ELEMENTS]; /* buffer for random info */
  } bin[RANDOM_SOURCES];
};

#if (CHIP == INTEL)
typedef unsigned reg_t;		/* machine register */

/* The stack frame layout is determined by the software, but for efficiency
 * it is laid out so the assembly code to use it is as simple as possible.
 * 80286 protected mode and all real modes use the same frame, built with
 * 16-bit registers.  Real mode lacks an automatic stack switch, so little
 * is lost by using the 286 frame for it.  The 386 frame differs only in
 * having 32-bit registers and more segment registers.  The same names are
 * used for the larger registers to avoid differences in the code.
 */
/*
 * 栈帧的布局布软件确定, 但是为考虑效率, 栈帧的布局是展开的, 于是汇编代码
 * 可尽可能简单地使用它们. 80286 保护模式和所有的实模式使用的是相同的帧,
 * 帧由16位的寄存器构建. 实模式无法自动切换栈, 所以使用 286 栈帧会损失一
 * 点信息. 386 的栈帧的不同之处仅在于它有32位的寄存器, 拥有更多的段寄存
 * 器. 在代码中为了避免差异, 对较长的寄存器使用了相同的名字.
 */
struct stackframe_s {           /* proc_ptr points here */
#if _WORD_SIZE == 4	// _WORD_SIZE ???
  // u16_t ???
  u16_t gs;                     /* last item pushed by save */
  u16_t fs;                     /*  ^ */
#endif
  u16_t es;                     /*  | */
  u16_t ds;                     /*  | */
  reg_t di;			/* di through cx are not accessed in C */
  reg_t si;			/* order is to match pusha/popa */
  reg_t fp;			/* bp */
  reg_t st;			/* hole for another copy of sp */
  reg_t bx;                     /*  | */
  reg_t dx;                     /*  | */
  reg_t cx;                     /*  | */
  reg_t retreg;			/* ax and above are all pushed by save */
  reg_t retadr;			/* return address for assembly code save() */
  // 同中断压栈的最后一项
  reg_t pc;			/*  ^  last item pushed by interrupt */
  reg_t cs;                     /*  | */
  // processor status word.
  reg_t psw;                    /*  | */
  // 堆栈指针
  reg_t sp;                     /*  | */
  reg_t ss;                     /* these are pushed by CPU during interrupt */
};
/* 
  31              24 23   22   21  20    19            16 15  14 13  12 11   8 7              0
  +-----------------+---+-----+---+-----+----------------+---+-----+---+------+----------------+
  | 基地址 31...24  | G | D/B | 0 | AVL | 段限长 19...16 | P | DPL | S | TYPE | 基地址 23...16 | 4
  +-----------------+---+-----+---+-----+----------------+---+-----+---+------+----------------+
  31                                                   16 15                                  0
  +------------------------------------------------------+-------------------------------------+
  | 基地址 15...0                                        | 段限长 15...0                       | 0
  +------------------------------------------------------+-------------------------------------+
  AVL	系统软件可用位				LIMIT	段限长 
  BASE	段基地址				P	段存在
  B/D	默认大小(0--16bit, 1--32bit)		S	描述符类型(0--系统, 1--代码或数据
  DPL	描述符特权级				TYPE	段类型 
  G	颗粒度	
*/
// 保护模式的段描述符
struct segdesc_s {		/* segment descriptor for protected mode */
// 段限长低 16 位
  u16_t limit_low;
  // 段基地址低 16 位
  u16_t base_low;
  // 段基地址高字的低 8 位
  u8_t base_middle;
  // 存在位, 描述符特权级, 描述符类型, 段类型
  u8_t access;			/* |P|DL|1|X|E|R|A| */
  // 颗粒度, 默认大小(0--16bit, 1-32bit), 系统软件可用位, 段限长高 4 位
  u8_t granularity;		/* |G|X|0|A|LIMT| */
  // 段基地址高字的高 8 位
  u8_t base_high;
};

typedef unsigned long irq_policy_t;	
typedef unsigned long irq_id_t;	

// 中断请求钩子
typedef struct irq_hook {
// 指向链表中的下一个元素
  struct irq_hook *next;		/* next hook in chain */
  // 中断服务程序
  int (*handler)(struct irq_hook *);	/* interrupt handler */
  // 中断向量号
  int irq;				/* IRQ vector number */ 
  // 钩子标识
  int id;				/* id of this hook */
  // 钩子的拥有者
  int proc_nr;				/* NONE if not in use */
  irq_id_t notify_id;			/* id to return on interrupt */
  // 策略掩码
  irq_policy_t policy;			/* bit mask for policy */
} irq_hook_t;

// irq_handler_t 是一个函数指针, 该指针所指向的函数拥有一
// struct irq_hook * 类型的参数, 返回值是 int 类型
typedef int (*irq_handler_t)(struct irq_hook *);

#endif /* (CHIP == INTEL) */

#if (CHIP == M68000)
/* M68000 specific types go here. */
#endif /* (CHIP == M68000) */

#endif /* TYPE_H */
