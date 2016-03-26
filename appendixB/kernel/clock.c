/* This file contains the clock task, which handles time related functions.
 * Important events that are handled by the CLOCK include setting and 
 * monitoring alarm timers and deciding when to (re)schedule processes. 
 * The CLOCK offers a direct interface to kernel processes. System services 
 * can access its services through system calls, such as sys_setalarm(). The
 * CLOCK task thus is hidden from the outside world.  
 *
 * Changes:
 *   Oct 08, 2005   reordering and comment editing (A. S. Woodhull)
 *   Mar 18, 2004   clock interface moved to SYSTEM task (Jorrit N. Herder) 
 *   Sep 30, 2004   source code documentation updated  (Jorrit N. Herder)
 *   Sep 24, 2004   redesigned alarm timers  (Jorrit N. Herder)
 *
 * The function do_clocktick() is triggered by the clock's interrupt 
 * handler when a watchdog timer has expired or a process must be scheduled. 
 *
 * In addition to the main clock_task() entry point, which starts the main 
 * loop, there are several other minor entry points:
 *   clock_stop:	called just before MINIX shutdown
 *   get_uptime:	get realtime since boot in clock ticks
 *   set_timer:		set a watchdog timer (+)
 *   reset_timer:	reset a watchdog timer (+)
 *   read_clock:	read the counter of channel 0 of the 8253A timer
 *
 * (+) The CLOCK task keeps tracks of watchdog timers for the entire kernel.
 * The watchdog functions of expired timers are executed in do_clocktick(). 
 * It is crucial that watchdog functions not block, or the CLOCK task may
 * be blocked. Do not send() a message when the receiver is not expecting it.
 * Instead, notify(), which always returns, should be used. 
 */
/*
 * 这个文件包含时钟任务, 该任务负责处理与时间有关的函数. 由 CLOCK
 * 处理的重要事件包括设置与操作闹钟定时器, 决定何时 (重新)调度进程.
 * CLOCK 向内核进程提供了一个非常直接的接口. 系统服务可以通过系统调用, 例如
 * sys_setalarm(), 来访问它的服务, 因此从外部看来 CLOCK 是被隐藏起来的.
 *
 * 更改日志:
 *	2005/10/08	对某些编辑进行了重排序与注释 (A. S. Woodhull)
 *	2004/3/18	将时钟接口移到 SYSTEM 任务 (Jorrit N. Herder)
 *	2004/9/30	更新了源代码文档 (Jorrit N. Herder)
 *	2004/9/24	重新设计了闹钟定时器 (Jorrit N.Herder)
 *
 * 当一个看门狗定时器到时, 或者某个进程将要被调度, 时钟中断处理程序将会调用
 * do_clocktick().
 *
 * clock_task() 在主循环开始部分有一个主要入口点, 除此之外还有几个次要
 * 入口点:
 *	clock_stop:	在 MINIX 关机前调用
 *	get_uptime:	返回从开机到现在到时钟滴答数
 *	set_timer:	设置一个看门狗定时器
 *	reset_timer:	重新设置一个看门狗定时器
 *	read_clock:	读取 8253A 通道 0 的计数器值
 *
 * 在整个内核中, CLOCK 任务都要始终了解看门狗定时器的状态. 到期的计时器的
 * 看门狗函数在 do_clocktick() 中执行. 看门狗函数不能阻塞, 否则的话 CLOCK 
 * 也可能阻塞, 这一点非常重要. 当接收方并不期望收到一个消息时, 不要给它
 * send() 消息. 相反, 应该使用 notify(), 这个函数总会返回.
 */

#include "kernel.h"
#include "proc.h"
#include <signal.h>
#include <minix/com.h>

/* Function prototype for PRIVATE functions. */ 
/* 私有函数的原型声明 */
// static void init_clock(void);
FORWARD _PROTOTYPE( void init_clock, (void) );
// static int clock_handler(irq_hook_t *hook);
FORWARD _PROTOTYPE( int clock_handler, (irq_hook_t *hook) );
// static int do_clocktick(message *m_ptr);
FORWARD _PROTOTYPE( int do_clocktick, (message *m_ptr) );

/* Clock parameters. */
// 使用方波的计数器频率
#define COUNTER_FREQ (2*TIMER_FREQ) /* counter frequency using square wave */
// latch: 锁存器
#define LATCH_COUNT     0x00	/* cc00xxxx, c = channel, x = any */
#define SQUARE_WAVE     0x36	/* ccaammmb, a = access, m = mode, b = BCD */
				/*   11x11, 11 = LSB then MSB, x11 = sq wave */

// 时钟的计数器值
#define TIMER_COUNT ((unsigned) (TIMER_FREQ/HZ)) /* initial value for counter*/

// 时钟频率
#define TIMER_FREQ  1193182L	/* clock frequency for timer in PC and AT */

// PS/2 的时钟中断响应位
#define CLOCK_ACK_BIT	0x80	/* PS/2 clock interrupt acknowledge bit */

/* The CLOCK's timers queue. The functions in <timers.h> operate on this. 
 * Each system process possesses a single synchronous alarm timer. If other 
 * kernel parts want to use additional timers, they must declare their own 
 * persistent (static) timer structure, which can be passed to the clock
 * via (re)set_timer().
 * When a timer expires its watchdog function is run by the CLOCK task. 
 */
/*
 * CLOCK 的定时器队列. <timers.h> 的函数会操作此队列. 每一个系统进程都拥有 
 * 一个单一同步的闹钟定时器. 如果内核中的其他部分想要使用一个额外的定时器,
 * 它们必须声明一个持久的(静态的)定时器结构, 这个结构可以通过
 * (re)set_timer() 传递给时钟.
 * 当一个定时器到期时, 它的看门狗函数将会被 CLOCK 任务调用.
 */
// CLOCK 的时钟队列头指针
PRIVATE timer_t *clock_timers;		/* queue of CLOCK timers */
// 下一个闹钟的超时值, 墙上时间
PRIVATE clock_t next_timeout;		/* realtime that next timer expires */

/* The time is incremented by the interrupt handler on each clock tick. */
// 墙上时间, 每次时钟中断都会增一
PRIVATE clock_t realtime;		/* real time clock */
// 中断处理程序钩子
PRIVATE irq_hook_t clock_hook;		/* interrupt handler hook */

/*===========================================================================*
 *				clock_task				     *
 *===========================================================================*/
PUBLIC void clock_task()
{
/* Main program of clock task. If the call is not HARD_INT it is an error.
 */
  message m;			/* message buffer for both input and output */
  int result;			/* result returned by the handler */

  // 初始化时钟
  init_clock();			/* initialize clock task */

  /* Main loop of the clock task.  Get work, process it. Never reply. */
  while (TRUE) {

      /* Go get a message. */
      receive(ANY, &m);	

      /* Handle the request. Only clock ticks are expected. */
      switch (m.m_type) {
      case HARD_INT:
          result = do_clocktick(&m);	/* handle clock tick */
          break;
      default:				/* illegal request type */
          kprintf("CLOCK: illegal request %d from %d.\n", m.m_type,m.m_source);
      }
  }
}

/*===========================================================================*
 *				do_clocktick				     *
 *===========================================================================*/
PRIVATE int do_clocktick(m_ptr)
message *m_ptr;				/* pointer to request message */
{
/* Despite its name, this routine is not called on every clock tick. It
 * is called on those clock ticks when a lot of work needs to be done.
 */

  /* A process used up a full quantum. The interrupt handler stored this
   * process in 'prev_ptr'.  First make sure that the process is not on the 
   * scheduling queues.  Then announce the process ready again. Since it has 
   * no more time left, it gets a new quantum and is inserted at the right 
   * place in the queues.  As a side-effect a new process will be scheduled.
   */ 
  if (prev_ptr->p_ticks_left <= 0 && priv(prev_ptr)->s_flags & PREEMPTIBLE) {
      lock_dequeue(prev_ptr);		/* take it off the queues */
      lock_enqueue(prev_ptr);		/* and reinsert it again */ 
  }

  /* Check if a clock timer expired and run its watchdog function. */
  if (next_timeout <= realtime) { 
  	tmrs_exptimers(&clock_timers, realtime, NULL);
  	next_timeout = clock_timers == NULL ? 
		TMR_NEVER : clock_timers->tmr_exp_time;
  }

  /* Inhibit sending a reply. */
  return(EDONTREPLY);
}

/*===========================================================================*
 *				init_clock				     *
 *===========================================================================*/
// 初始化时钟
PRIVATE void init_clock()
{
  /* Initialize the CLOCK's interrupt hook. */
	// 时钟钩子的拥有者为 CLOCK
  clock_hook.proc_nr = CLOCK;

  /* Initialize channel 0 of the 8253A timer to, e.g., 60 Hz. */
  // 设置 8253A 的计时模式, 方波
  outb(TIMER_MODE, SQUARE_WAVE);	/* set timer to run continuously */
  // 写计数器值, 先写低 8 位, 再写高 8 位
  outb(TIMER0, TIMER_COUNT);		/* load timer low byte */
  outb(TIMER0, TIMER_COUNT >> 8);	/* load timer high byte */
  put_irq_handler(&clock_hook, CLOCK_IRQ, clock_handler);/* register handler */
  enable_irq(&clock_hook);		/* ready for clock interrupts */
}

/*===========================================================================*
 *				clock_stop				     *
 *===========================================================================*/
// 停止时钟.
PUBLIC void clock_stop()
{
/* Reset the clock to the BIOS rate. (For rebooting) */
  outb(TIMER_MODE, 0x36);
  outb(TIMER0, 0);
  outb(TIMER0, 0);
}

/*===========================================================================*
 *				clock_handler				     *
 *===========================================================================*/
// 时钟中断处理程序, 函数的定义形式为 C 语言的旧风格, 为向下兼容
// ANSI 仍支持. 当时钟中断发生时, 就会调用该函数. 函数会更新系统的运行时间,
// 进程的运行时间(用户态时间或系统态时间)与运行时间片. 如果有定时器到期, 或
// 当前进程时间片用完, 则向时钟任务发通知.
PRIVATE int clock_handler(hook)
irq_hook_t *hook;
{
/* This executes on each clock tick (i.e., every time the timer chip generates 
 * an interrupt). It does a little bit of work so the clock task does not have 
 * to be called on every tick.  The clock task is called when:
 *
 *	(1) the scheduling quantum of the running process has expired, or
 *	(2) a timer has expired and the watchdog function should be run.
 *
 * Many global global and static variables are accessed here.  The safety of
 * this must be justified. All scheduling and message passing code acquires a 
 * lock by temporarily disabling interrupts, so no conflicts with calls from 
 * the task level can occur. Furthermore, interrupts are not reentrant, the 
 * interrupt handler cannot be bothered by other interrupts.
 * 
 * Variables that are updated in the clock's interrupt handler:
 *	lost_ticks:
 *		Clock ticks counted outside the clock task. This for example
 *		is used when the boot monitor processes a real mode interrupt.
 * 	realtime:
 * 		The current uptime is incremented with all outstanding ticks.
 *	proc_ptr, bill_ptr:
 *		These are used for accounting.  It does not matter if proc.c
 *		is changing them, provided they are always valid pointers,
 *		since at worst the previous process would be billed.
 */
	/*
	 * 每产生一个时钟滴答(即定时器芯片每产生一次中断), 这个函数就会被
	 * 调用. 函数只做很少的工作, 于是时钟任务不用在每个滴答都要被调用.
	 * 当下行条件发生时, 时钟任务被调用:
	 *	(1) 正在运行的进程的调度量子(时间片)已经用完;
	 *	(2) 某个定时器到期, 需要调用看门狗函数.
	 * 函数会访问许多全局与静态变量. 必须严格保证安全. 所有的调度和消息
	 * 传递代码都会要求获得锁, 这是通过临时禁止中断实现的, 所有在任务
	 * 层次上不会有冲突产生. 另外, 中断是不可重入的, 所以中断服务程序
	 * 不能被其它中断打断.
	 *
	 * 这个函数会修改的变量包括:
	 *	lost_ticks: 在时钟任务之外计数的时钟滴答数. 当引导监视程序
	 * 处理一个实模式中断时会使用到.
	 *	realtime: 当前的计算机正常运行时间, 会被所有的未决的滴答
	 * 递增.
	 *	proc_ptr, bill_ptr: 这两个变量用于记帐. 如果 proc.c 改变了
	 * 它们也无关紧要, 假设它们的值总是有效的, 最坏情况不过是把前一个
	 * 进程也算在内.
	 */
  register unsigned ticks;

  /* Acknowledge the PS/2 clock interrupt. */
  /* 响应 PS/2 的时钟中断 */
  if (machine.ps_mca) outb(PORT_B, inb(PORT_B) | CLOCK_ACK_BIT);

  /* Get number of ticks and update realtime. */
  ticks = lost_ticks + 1;
  lost_ticks = 0;
  realtime += ticks;

  /* Update user and system accounting times. Charge the current process for
   * user time. If the current process is not billable, that is, if a non-user
   * process is running, charge the billable process for system time as well.
   * Thus the unbillable process' user time is the billable user's system time.
   */
  /*
   * 更新用户与系统的时间会计信息. 为当前进程记录用户态时间信息, 如果当前
   * 进程是不可记账的, 也就是说当前没有用户态进程在运行, 将时间记在系统态
   * 上. 于是不可记帐进程的用户态运行时间等于可记账进程的系统态运行时间.
   */
  // 更新当前进程的用户态运行时间.
  proc_ptr->p_user_time += ticks;
  // 如果当前进程的特权级结构设置了可抢占标志, 减少它的剩余
  // 调度时间片.
  if (priv(proc_ptr)->s_flags & PREEMPTIBLE) {
      proc_ptr->p_ticks_left -= ticks;
  }
  // 如果当前进程的特权级结构没有设置可记帐标志, 即当前进程是不可记帐的,
  // 更新当前进程的系统态运行时间, 同时减少它的剩余调度时间片.
  if (! (priv(proc_ptr)->s_flags & BILLABLE)) {
      bill_ptr->p_sys_time += ticks;
      bill_ptr->p_ticks_left -= ticks;
  }

  /* Check if do_clocktick() must be called. Done for alarms and scheduling.
   * Some processes, such as the kernel tasks, cannot be preempted. 
   */ 
  /*
   * 检查是否需要调用 do_clocktick(). 如果是闹钟到期或者需要调度, 则调用 
   * do_clocktick(). 某些进程, 例如内核任务, 是不可抢占的.
   */
  // 
  if ((next_timeout <= realtime) || (proc_ptr->p_ticks_left <= 0)) {
      prev_ptr = proc_ptr;			/* store running process */
      // 向 CLOCK 发送通知
      lock_notify(HARDWARE, CLOCK);		/* send notification */
  } 
  return(1);					/* reenable interrupts */
}

/*===========================================================================*
 *				get_uptime				     *
 *===========================================================================*/
// 返回从系统开机到当前时间经历过的时钟滴答数
PUBLIC clock_t get_uptime()
{
/* Get and return the current clock uptime in ticks. */
  return(realtime);
}

/*===========================================================================*
 *				set_timer				     *
 *===========================================================================*/
// 将一个新的时钟插入时钟队列中, 并更新下一次的超时时间.
PUBLIC void set_timer(tp, exp_time, watchdog)
struct timer *tp;		/* pointer to timer structure */
clock_t exp_time;		/* expiration realtime */
tmr_func_t watchdog;		/* watchdog to be called */
{
/* Insert the new timer in the active timers list. Always update the 
 * next timeout time by setting it to the front of the active list.
 */
/*
 * 将一个新的时钟插入活动的时钟列表中. 始终更新下一次的超时时间, 就是将时钟
 * 队头的到期时间赋给 next_timeout 完成.
 */
	// tmrs_settimer() 由库函数提供, 不包括在内核中
  tmrs_settimer(&clock_timers, tp, exp_time, watchdog, NULL);
  next_timeout = clock_timers->tmr_exp_time;
}

/*===========================================================================*
 *				reset_timer				     *
 *===========================================================================*/
// 将指定的时钟从队列移除, 并更新下一次的超时时间.
PUBLIC void reset_timer(tp)
struct timer *tp;		/* pointer to timer structure */
{
/* The timer pointed to by 'tp' is no longer needed. Remove it from both the
 * active and expired lists. Always update the next timeout time by setting
 * it to the front of the active list.
 */
// 将 tp 从时钟队列中移走. 更新下一次超时时间, 如果队列为空, 则将下一次的到
// 其时间设置成一个永远不会到期的时间.
  tmrs_clrtimer(&clock_timers, tp, NULL);
  next_timeout = (clock_timers == NULL) ? 
	TMR_NEVER : clock_timers->tmr_exp_time;
}

/*===========================================================================*
 *				read_clock				     *
 *===========================================================================*/
// 读取计时器芯片当前的计数值.
PUBLIC unsigned long read_clock()
{
/* Read the counter of channel 0 of the 8253A timer.  This counter counts
 * down at a rate of TIMER_FREQ and restarts at TIMER_COUNT-1 when it
 * reaches zero. A hardware interrupt (clock tick) occurs when the counter
 * gets to zero and restarts its cycle.  
 */
/*
 * 读取 8253A 通道0 的计数器值. 这个计数器的值以 TIMER_FREQ 的速度递减,
 * 递减到 0 时重新从 TIMER_COUNT-1 开始. 当递减到 0 时计时芯片会产生一个
 * 时钟中断.
 */
  unsigned count;

  // 锁存当前计数值
  outb(TIMER_MODE, LATCH_COUNT);
  // 读取锁存器的值, 先低 8 位, 再读高8 位.
  count = inb(TIMER0);
  count |= (inb(TIMER0) << 8);
  
  return count;
}
