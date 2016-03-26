/* This file contains the main program of MINIX as well as its shutdown code.
 * The routine main() initializes the system and starts the ball rolling by
 * setting up the process table, interrupt vectors, and scheduling each task 
 * to run to initialize itself.
 * The routine shutdown() does the opposite and brings down MINIX. 
 *
 * The entries into this file are:
 *   main:	    	MINIX main program
 *   prepare_shutdown:	prepare to take MINIX down
 *
 * Changes:
 *   Nov 24, 2004   simplified main() with system image  (Jorrit N. Herder)
 *   Aug 20, 2004   new prepare_shutdown() and shutdown()  (Jorrit N. Herder)
 */
/* 该文件包含 MINIX 的主程序, 以及关机代码. main() 函数初始化系统, 开始
 * 滚动球(???), 通过设置进程表, 中断向量, 调度每一个任务开始运行以初始化
 * 自身.
 * shutdown() 的功能与 main() 相反.
 * 该文件的条目包括:
 *	main:			MINIX 主程序;
 *	prepare_shutdown:	为 MINIX 关机作准备.
 *
 * 更改:
 *	2004/11/24 简化 main();
 *	2004/08/20 新的 prepare_shutdown() 和 shutdown().
 */
#include "kernel.h"
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <a.out.h>
#include <minix/callnr.h>
#include <minix/com.h>
#include "proc.h"

/* Prototype declarations for PRIVATE functions. */
// 应该是旧风格的 C 函数声明形式
// 'FORWARD' for 'static'
FORWARD _PROTOTYPE( void announce, (void));	
FORWARD _PROTOTYPE( void shutdown, (timer_t *tp));

/*===========================================================================*
 *				main                                         *
 *===========================================================================*/
PUBLIC void main()
{
/* Start the ball rolling. */
  struct boot_image *ip;	/* boot image pointer */
  register struct proc *rp;	/* process pointer */
  register struct priv *sp;	/* privilege structure pointer */
  register int i, s;
  // a.out 头部数组的索引.
  int hdrindex;			/* index to array of a.out headers */
  phys_clicks text_base;
  vir_clicks text_clicks, data_clicks;
  // 内核任务栈的基地址(低端)
  reg_t ktsb;			/* kernel task stack base */
  // 用来放置 a.out 头部的一个副本.
  struct exec e_hdr;		/* for a copy of an a.out header */

  /* Initialize the interrupt controller. */
  // 初始化 8259 中断控制器芯片.
  intr_init(1);

  /* Clear the process table. Anounce each slot as empty and set up mappings 
   * for proc_addr() and proc_nr() macros. Do the same for the table with 
   * privilege structures for the system processes. 
   */
  // 初如化进程表与进程指针表.
  // BEG_PROC_ADDR: 进程表地址;
  for (rp = BEG_PROC_ADDR, i = -NR_TASKS; rp < END_PROC_ADDR; ++rp, ++i) {
	  // 将进程表中每一项都设置为空闲.
  	rp->p_rts_flags = SLOT_FREE;		/* initialize free slot */
	// 进程号, i 的初值为 -NR_TASKS, 可见系统任务拥有负的进程号
	rp->p_nr = i;				/* proc number from ptr */
	// 建立进程数组与进程指针数组之间的映射关系
        (pproc_addr + NR_TASKS)[i] = rp;        /* proc ptr from number */
  }
  // 初始化优先级表
  for (sp = BEG_PRIV_ADDR, i = 0; sp < END_PRIV_ADDR; ++sp, ++i) {
	sp->s_proc_nr = NONE;			/* initialize as free */
	sp->s_id = i;				/* priv structure index */
	// 建立特权级表与特权级指针表之间的映射关系
	ppriv_addr[i] = sp;			/* priv ptr from number */
  }

  /* Set up proc table entries for tasks and servers.  The stacks of the
   * kernel tasks are initialized to an array in data space.  The stacks
   * of the servers have been added to the data segment by the monitor, so
   * the stack pointer is set to the end of the data segment.  All the
   * processes are in low memory on the 8086.  On the 386 only the kernel
   * is in low memory, the rest is loaded in extended memory.
   */
  /*
   * 为任务和服务进程设置进程表项. 内核任务的栈被初始化成一个在数据空间中的
   * 数组. 服务进程的栈已经由控制器添加到数据段中, 所有它们的栈指针开始时
   * 指向数据段的末尾. 所有的进程都在 8086 的低内存. 对于 386, 只有内核在
   * 低内存, 剩下的都在扩展内存中.
   */

  /* Task stacks. */
  /* 任务栈 */
  ktsb = (reg_t) t_stack;
 // 为那些包含在系统引导映像文件中的程序分配进程表项.
  for (i=0; i < NR_BOOT_PROCS; ++i) {
	ip = &image[i];				/* process' attributes */
	// 获取进程指针
	rp = proc_addr(ip->proc_nr);		/* get process pointer */
	// 最大调度优先级
	rp->p_max_priority = ip->priority;	/* max scheduling priority */
	// 当前调度优先级
	rp->p_priority = ip->priority;		/* current priority */
	// 时间片原子值
	rp->p_quantum_size = ip->quantum;	/* quantum size in ticks */
	// 剩余时间片
	rp->p_ticks_left = ip->quantum;		/* current credit */
	// 将程序名复制到进程表项中
	strncpy(rp->p_name, ip->proc_name, P_NAME_LEN); /* set process name */
	// 为进程分配一个特权级结构体, 即 从系统特权级表中分配一项
	(void) get_priv(rp, (ip->flags & SYS_PROC));    /* assign structure */
	// 初始化特权级结构体的标志.
	priv(rp)->s_flags = ip->flags;			/* process flags */
	// 初始化特权级结构体的 允许的系统调用陷井
	priv(rp)->s_trap_mask = ip->trap_mask;		/* allowed traps */
	priv(rp)->s_call_mask = ip->call_mask;		/* kernel call mask */
	// 初始化进程的消息发送位图
	priv(rp)->s_ipc_to.chunk[0] = ip->ipc_to;	/* restrict targets */
	// 如果进程是内核任务
	if (iskerneln(proc_nr(rp))) {		/* part of the kernel? */ 
		// 如果进程的栈大小大于 0, 设置进程的栈警戒字, 
		if (ip->stksize > 0) {		/* HARDWARE stack size is 0 */
			// 设置内核任务栈警戒字指针.
			rp->p_priv->s_stack_guard = (reg_t *) ktsb;
			// 指针运算符(->) 要比取值运行符 (*) 的优先级要高.
			// 等价于: 
			// *(rp->p_priv->s_stack_guard) = STACK_GUARD
			// 效果是在栈的最顶端(在低地址)放置一个特殊值,
			// 这个值就是栈警戒字.
			*rp->p_priv->s_stack_guard = STACK_GUARD;
		}
		ktsb += ip->stksize;	/* point to high end of stack */
		// 初始进程的栈指针
		rp->p_reg.sp = ktsb;	/* this task's initial stack ptr */
		// kinfo ???
		// 内核代码的基地址右移 CLICK_SHIFT 位, 赋给 text_base.
		text_base = kinfo.code_base >> CLICK_SHIFT;
					/* processes that are in the kernel */
		// 内核任务使用同一个 a.out 头部信息
		hdrindex = 0;		/* all use the first a.out header */
	} else {
		// 非内核任务, 计算它的 a.out 头部数组索引, 因为 0 号项
		// 留给了内核任务, 所以需 加 1.
		hdrindex = 1 + i-NR_TASKS;	/* servers, drivers, INIT */
	}

	/* The bootstrap loader created an array of the a.out headers at
	 * absolute address 'aout'. Get one element to e_hdr.
	 */
	/*
	 * 引导加载程序会在绝对地址 'aout' 处放置一个 a.out 头部数组.
	 * 从中取一项复制到 e_hdr.
	 */
	phys_copy(aout + hdrindex * A_MINHDR, vir2phys(&e_hdr),
						(phys_bytes) A_MINHDR);
	/* Convert addresses to clicks and build process memory map */
	/* 将地址转换为以 click 为单位, 并建立进程内存映射 */
	// 既然这里要设置 text_base, 那 146 行附近的 
	// text_base = kinfo.code_base >> CLICK_SHIFT;
	// 岂不是多余的??
	// 将 a.out 头部的符号表大小右移 CLICK_SHIFT 位,赋给 text_base.
	text_base = e_hdr.a_syms >> CLICK_SHIFT;
	// 计算程序文本段大小, 以 click 为单位, 上取整.
	text_clicks = (e_hdr.a_text + CLICK_SIZE-1) >> CLICK_SHIFT;
	// 如果 a.out 头部指明它的 I/D 是合并的 ???
	if (!(e_hdr.a_flags & A_SEP)) text_clicks = 0;	   /* common I&D */
	// 计算程序占用的内存量, 以 click 为单位, 上取整.
	data_clicks = (e_hdr.a_total + CLICK_SIZE-1) >> CLICK_SHIFT;
	// 初始化进程的内存映射数据结构
	rp->p_memmap[T].mem_phys = text_base;
	rp->p_memmap[T].mem_len  = text_clicks;
	rp->p_memmap[D].mem_phys = text_base + text_clicks;
	rp->p_memmap[D].mem_len  = data_clicks;
	rp->p_memmap[S].mem_phys = text_base + text_clicks + data_clicks;
	rp->p_memmap[S].mem_vir  = data_clicks;	/* empty - stack is in data */

	/* Set initial register values.  The processor status word for tasks 
	 * is different from that of other processes because tasks can
	 * access I/O; this is not allowed to less-privileged processes 
	 */
	/*
	 * 设置寄存器的初始值. 与其他进程相比, 内核任务的处理器状态字
	 * 稍有不同, 因为内核任务可以访问 I/O; 而对于非特权进来来说, 这是不
	 * 允许的.
	 */
	// 初始化进程的 PC 和 processor status word.
	rp->p_reg.pc = (reg_t) ip->initial_pc;
	rp->p_reg.psw = (iskernelp(rp)) ? INIT_TASK_PSW : INIT_PSW;

	/* Initialize the server stack pointer. Take it down one word
	 * to give crtso.s something to use as "argc".
	 */
	/*
	 * 初始化服务器进程的栈指针. 下移一个字的空间, 使得 crtso.s 有
	 * 空间放置 "argc".
	 */
	if (isusern(proc_nr(rp))) {		/* user-space process? */ 
		rp->p_reg.sp = (rp->p_memmap[S].mem_vir +
				rp->p_memmap[S].mem_len) << CLICK_SHIFT;
		rp->p_reg.sp -= sizeof(reg_t);
	}
	
	/* Set ready. The HARDWARE task is never ready. */
	if (rp->p_nr != HARDWARE) {
		// 如果进程不是 HARDWARE, 清空进程标志, 并加入调度队列.
		rp->p_rts_flags = 0;		/* runnable if no flags */
		lock_enqueue(rp);		/* add to scheduling queues */
	} else {
		// 对于  HARDWARE 任务, 则阻止其运行. ???
		rp->p_rts_flags = NO_MAP;	/* prevent from running */
	}

	/* Code and data segments must be allocated in protected mode. */
	/* 数据与代码段必须在保护模式下分配 */
	alloc_segments(rp);
  }

  /* We're definitely not shutting down. */
  /* 我们清楚地知道不能关机 */
  shutdown_started = 0;

  /* MINIX is now ready. All boot image processes are on the ready queue.
   * Return to the assembly code to start running the current process. 
   */
  /*
   * MINIX 现在准备好了. 所有的引导映像文件中的进程都处在就绪队列中.
   * 回到汇编代码, 开始运行当前进程.
   */
  bill_ptr = proc_addr(IDLE);		/* it has to point somewhere */
  // 打印 MINIX 的启动标语
  announce();				/* print MINIX startup banner */
  // 重启 当前进程(下一个可运行进程), 不再返回
  restart();	// _restart
}

/*===========================================================================*
 *				announce				     *
 *===========================================================================*/
// 显示 MINIX 的启动标语.
PRIVATE void announce(void)
{
  /* Display the MINIX startup banner. */
  kprintf("MINIX %s.%s."  
      "Copyright 2006, Vrije Universiteit, Amsterdam, The Netherlands\n", 
      OS_RELEASE, OS_VERSION);

  /* Real mode, or 16/32-bit protected mode? */
  kprintf("Executing in %s mode.\n\n",
      machine.protected ? "32-bit protected" : "real");
}

/*===========================================================================*
 *				prepare_shutdown			     *
 *===========================================================================*/
// 关机前的准备
PUBLIC void prepare_shutdown(how)
int how;
{
/* This function prepares to shutdown MINIX. */
  static timer_t shutdown_timer;
  register struct proc *rp; 
  message m;

  /* Show debugging dumps on panics. Make sure that the TTY task is still 
   * available to handle them. This is done with help of a non-blocking send. 
   * We rely on TTY to call sys_abort() when it is done with the dumps.
   */
  /*
   * 在崩溃时显示调试信息. 确保 TTY 任务仍然可以显示信息. 这个工作是在非
   * 阻塞发送的帮助下完成的. 当调试信息显示完成后, 我们依赖 TTY 调用
   * sys_abort().
   */
  if (how == RBT_PANIC) {
      m.m_type = PANIC_DUMPS;
      // 以非阻塞的方式给 TTY 发送消息, 消息的类型是崩溃信息导出.
      // /* ??? */
      if (nb_send(TTY_PROC_NR,&m)==OK)	/* don't block if TTY isn't ready */
          return;			/* await sys_abort() from TTY */
  }

  /* Send a signal to all system processes that are still alive to inform 
   * them that the MINIX kernel is shutting down. A proper shutdown sequence
   * should be implemented by a user-space server. This mechanism is useful
   * as a backup in case of system panics, so that system processes can still
   * run their shutdown code, e.g, to synchronize the FS or to let the TTY
   * switch to the first console. 
   */
  /*
   * 给所有仍然活动的系统进程发送信号, 通知它们内核就要关闭. 一个适当的停机
   * 顺序应该由一个用户空间的服务器进程来实现. 当系统崩溃时, 这是一个非常有
   * 用的备份机制, 于是系统进程仍然可以运行它们的停机代码, 举例来说, 同步
   * 文件系统, 让 TTY 切换到第一个控制台.
   */
  kprintf("Sending SIGKSTOP to system processes ...\n"); 
  for (rp=BEG_PROC_ADDR; rp<END_PROC_ADDR; rp++) {
	  // 如果进程是带特权的系统进程, 并且不是内核任务, 则给进程
	  // 发送信号 SIGKSTOP.
      if (!isemptyp(rp) && (priv(rp)->s_flags & SYS_PROC) && !iskernelp(rp))
          send_sig(proc_nr(rp), SIGKSTOP);
  }

  /* We're shutting down. Diagnostics may behave differently now. */
  shutdown_started = 1;

  /* Notify system processes of the upcoming shutdown and allow them to be 
   * scheduled by setting a watchog timer that calls shutdown(). The timer 
   * argument passes the shutdown status. 
   */
  /*
   * 通知系统进程即将到来的停机, 以允许它们被调度, 通过设置一个看门
   * 狗定时器并调用 shutdown(). 定时器的参数传递了关机状态.
   */
  kprintf("MINIX will now be shut down ...\n");
  tmr_arg(&shutdown_timer)->ta_int = how;

  /* Continue after 1 second, to give processes a chance to get
   * scheduled to do shutdown work.
   */
  /*
   * 1 秒钟后继续, 使得进程有机会被调度来做停机工作.
   */
  set_timer(&shutdown_timer, get_uptime() + HZ, shutdown);
}

/*===========================================================================*
 *				shutdown 				     *
 *===========================================================================*/
PRIVATE void shutdown(tp)
timer_t *tp;
{
/* This function is called from prepare_shutdown or stop_sequence to bring 
 * down MINIX. How to shutdown is in the argument: RBT_HALT (return to the
 * monitor), RBT_MONITOR (execute given code), RBT_RESET (hard reset). 
 */
  int how = tmr_arg(tp)->ta_int;
  u16_t magic; 

  /* Now mask all interrupts, including the clock, and stop the clock. */
  /* 屏蔽所有的中断, 包括时钟中断, 然后再停止时钟 */
  outb(INT_CTLMASK, ~0); 
  clock_stop();

  // 如果我们可以回到控制台, 且不是硬件复位.
  if (mon_return && how != RBT_RESET) {
	/* Reinitialize the interrupt controllers to the BIOS defaults. */
	  /* 将中断控制器重新初始化成 BIOS 的默认状态 */
	intr_init(0);
	// 禁止中断
	outb(INT_CTLMASK, 0);
	outb(INT2_CTLMASK, 0);

	/* Return to the boot monitor. Set the program if not already done. */
	/* 回到引导监视器. 设置程序, 如果它们没有准备好的话. */
	// 给内核信息传递一个空字符
	if (how != RBT_MONITOR) phys_copy(vir2phys(""), kinfo.params_base, 1); 
	// 在特权级 0 的状态下调用 monitor
	level0(monitor); // _monitor _level0
  }

  /* Reset the system by jumping to the reset address (real mode), or by
   * forcing a processor shutdown (protected mode). First stop the BIOS 
   * memory test by setting a soft reset flag. 
   */
  /*
   * 通过跳转到实模式下的复位地址来复位系统,
   * 或者通过在保护模式下强制处理器关机来复位. 首先通过设置一个软复位标志
   * 来禁止 BIOS 的内存检查.
   */
  magic = STOP_MEM_CHECK;
  // 将标志复制到特定的位置
  phys_copy(vir2phys(&magic), SOFT_RESET_FLAG_ADDR, SOFT_RESET_FLAG_SIZE);
  // 在特权级 0 的状态下调用 _reset.	_level0
  level0(reset);
}

