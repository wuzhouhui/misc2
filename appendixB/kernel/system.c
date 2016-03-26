/* This task provides an interface between the kernel and user-space system
 * processes. System services can be accessed by doing a kernel call. Kernel
 * calls are  transformed into request messages, which are handled by this
 * task. By convention, a sys_call() is transformed in a SYS_CALL request
 * message that is handled in a function named do_call(). 
 *
 * A private call vector is used to map all kernel calls to the functions that
 * handle them. The actual handler functions are contained in separate files
 * to keep this file clean. The call vector is used in the system task's main
 * loop to handle all incoming requests.  
 *
 * In addition to the main sys_task() entry point, which starts the main loop,
 * there are several other minor entry points:
 *   get_priv:		assign privilege structure to user or system process
 *   send_sig:		send a signal directly to a system process
 *   cause_sig:		take action to cause a signal to occur via PM
 *   umap_local:	map virtual address in LOCAL_SEG to physical 
 *   umap_remote:	map virtual address in REMOTE_SEG to physical 
 *   umap_bios:		map virtual address in BIOS_SEG to physical 
 *   virtual_copy:	copy bytes from one virtual address to another 
 *   get_randomness:	accumulate randomness in a buffer
 *
 * Changes:
 *   Aug 04, 2005   check if kernel call is allowed  (Jorrit N. Herder)
 *   Jul 20, 2005   send signal to services with message  (Jorrit N. Herder) 
 *   Jan 15, 2005   new, generalized virtual copy function  (Jorrit N. Herder)
 *   Oct 10, 2004   dispatch system calls from call vector  (Jorrit N. Herder)
 *   Sep 30, 2004   source code documentation updated  (Jorrit N. Herder)
 */
/*
 * 这个任务在内核与用户空间的系统进程之间提供了一个接口.
 * 系统服务可以通过系统调用访问. 系统调用被传递到请求消息中, 而请求消息会被
 * 该任务处理. 为方便起见, 一个 sys_call() 会被传递到 SYS_CALL 请求消息中,
 * 该请求消息会被 do_call() 处理.
 *
 * 一个私有的调用向量表用来将所有的内核调用映射到它们的处理函数. 真正的处理
 * 函数会包含在几个单独的文件中, 否则这个文件会很混乱. 这个调用向量在系统
 * 任务的主循环中使用, 处理所有到来的请求.
 *
 * 除了主要的入口点 sys_task(), 该入口点在主循环开始, 还有其他几个次要
 * 的入口点:
 *	get_priv:	为用户进程与系统进程分配特权级结构;
 *	send_sig:	直接向一个系统进程发送信号;
 *	cause_sig:	为导致一个信号发生而采取行动, 通过 PM;
 *	umap_local:	将 LOCAL_SEG 的虚拟地址映射到物理地址;
 *	umap_remote:	将 REMOTE_SEG 的虚拟地址映射到物理地址;
 *	virtual_copy:	从一个虚拟地址复制数据到另一个虚拟地址;
 *	get_randomness:	在一个缓冲区中累积随机性.
 *
 * 更改:
 *	....
 */

#include "kernel.h"
#include "system.h"
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/sigcontext.h>
#include <ibm/memory.h>
#include "protect.h"

/* Declaration of the call vector that defines the mapping of kernel calls 
 * to handler functions. The vector is initialized in sys_init() with map(), 
 * which makes sure the kernel call numbers are ok. No space is allocated, 
 * because the dummy is declared extern. If an illegal call is given, the 
 * array size will be negative and this won't compile. 
 */
/*
 * 声明一个调用向量表, 该表定义了系统调用到处理函数的映射关系. 这个向量表
 * 在sys_init() 中调用 map() 初始化, map() 确保系统调用号是正确的. 没有 
 * 分配存储空间, 因为 dummy 被声明为 extern.
 * 如果系统调用是非法的, 那么数组的大小就会是负的, 编译就无法通过.
 */
PUBLIC int (*call_vec[NR_SYS_CALLS])(message *m_ptr);

// 将系统调用号映射到它的处理函数, 在映射之前检查调用号的合法性
#define map(call_nr, handler) \
    {extern int dummy[NR_SYS_CALLS>(unsigned)(call_nr-KERNEL_CALL) ? 1:-1];} \
    call_vec[(call_nr-KERNEL_CALL)] = (handler)  

FORWARD _PROTOTYPE( void initialize, (void));

/*===========================================================================*
 *				sys_task				     *
 *===========================================================================*/
PUBLIC void sys_task()
{
/* Main entry point of sys_task.  Get the message and dispatch on type. */
/* 系统调用的主要入口点. 获取消息, 再根据类型分派. */
  static message m;
  register int result;
  register struct proc *caller_ptr;
  unsigned int call_nr;
  int s;

  /* Initialize the system task. */
  /* 初始化系统任务. */ // 初始化系统调用
  initialize();

  while (TRUE) {
      /* Get work. Block and wait until a request message arrives. */
	/* 开始工作. 阻塞, 直到一个请求消息到来. */
      receive(ANY, &m);			
      // 系统调用向量号
      call_nr = (unsigned) m.m_type - KERNEL_CALL;	
      // 请求系统调用的进程指针
      caller_ptr = proc_addr(m.m_source);	

      /* See if the caller made a valid request and try to handle it. */
      // 检查主调进程是否有能力请求该系统调用.
      if (! (priv(caller_ptr)->s_call_mask & (1<<call_nr))) {
	  kprintf("SYSTEM: request %d from %d denied.\n", call_nr,m.m_source);
	  result = ECALLDENIED;			/* illegal message type */
      } else if (call_nr >= NR_SYS_CALLS) {		/* check call number */
	      // 检查系统调用向量号的合法性.
	  kprintf("SYSTEM: illegal request %d from %d.\n", call_nr,m.m_source);
	  result = EBADREQUEST;			/* illegal message type */
      } 
      else {
      // 执行系统调用.
          result = (*call_vec[call_nr])(&m);	/* handle the kernel call */
      }

      /* Send a reply, unless inhibited by a handler function. Use the kernel
       * function lock_send() to prevent a system call trap. The destination
       * is known to be blocked waiting for a message.
       */
      /*
       * 发送一个回复, 除非被系统调用处理函数禁止了. 使得系统函数
       * lock_send() 来禁止系统调用陷入. 已知目标进程因为等待一个消息而
       * 阻塞.
       */
      if (result != EDONTREPLY) {
	      // 将系统调用的执行结果发送给系统调用的主调进程.
  	  m.m_type = result;			/* report status of call */
          if (OK != (s=lock_send(m.m_source, &m))) {
              kprintf("SYSTEM, reply to %d failed: %d\n", m.m_source, s);
          }
      }
  }
}

/*===========================================================================*
 *				initialize				     *
 *===========================================================================*/
// 初始化系统调用
PRIVATE void initialize(void)
{
  register struct priv *sp;
  int i;

  /* Initialize IRQ handler hooks. Mark all hooks available. */
  /* 初始化中断请求处理函数钩子. 把所有的钩子标记为可用. */
  for (i=0; i<NR_IRQ_HOOKS; i++) {
      irq_hooks[i].proc_nr = NONE;
  }

  /* Initialize all alarm timers for all processes. */
  /* 为所有进程初始化所有的闹钟定时器. */
  for (sp=BEG_PRIV_ADDR; sp < END_PRIV_ADDR; sp++) {
    tmr_inittimer(&(sp->s_alarm_timer));
  }

  /* Initialize the call vector to a safe default handler. Some kernel calls 
   * may be disabled or nonexistant. Then explicitly map known calls to their
   * handler functions. This is done with a macro that gives a compile error
   * if an illegal call number is used. The ordering is not important here.
   */
  /*
   * 将调用向量表初始化到一个默认的安全的处理程序. 一些系统调用可能被禁止
   * 或根本就不存在. 然后再显式地将已经的系统调用映射到对应 的处理程序.
   * 这个工作由一个宏完成, 如果参数不合法, 就会产生一个编译错误. 顺序并
   * 不重要.
   */
  for (i=0; i<NR_SYS_CALLS; i++) {
      call_vec[i] = do_unused;
  }

  /* Process management. */
  map(SYS_FORK, do_fork); 		/* a process forked a new process */
  map(SYS_EXEC, do_exec);		/* update process after execute */
  map(SYS_EXIT, do_exit);		/* clean up after process exit */
  map(SYS_NICE, do_nice);		/* set scheduling priority */
  map(SYS_PRIVCTL, do_privctl);		/* system privileges control */
  map(SYS_TRACE, do_trace);		/* request a trace operation */

  /* Signal handling. */
  map(SYS_KILL, do_kill); 		/* cause a process to be signaled */
  map(SYS_GETKSIG, do_getksig);		/* PM checks for pending signals */
  map(SYS_ENDKSIG, do_endksig);		/* PM finished processing signal */
  map(SYS_SIGSEND, do_sigsend);		/* start POSIX-style signal */
  map(SYS_SIGRETURN, do_sigreturn);	/* return from POSIX-style signal */

  /* Device I/O. */
  map(SYS_IRQCTL, do_irqctl);  		/* interrupt control operations */ 
  map(SYS_DEVIO, do_devio);   		/* inb, inw, inl, outb, outw, outl */ 
  map(SYS_SDEVIO, do_sdevio);		/* phys_insb, _insw, _outsb, _outsw */
  map(SYS_VDEVIO, do_vdevio);  		/* vector with devio requests */ 
  map(SYS_INT86, do_int86);  		/* real-mode BIOS calls */ 

  /* Memory management. */
  map(SYS_NEWMAP, do_newmap);		/* set up a process memory map */
  map(SYS_SEGCTL, do_segctl);		/* add segment and get selector */
  map(SYS_MEMSET, do_memset);		/* write char to memory area */

  /* Copying. */
  map(SYS_UMAP, do_umap);		/* map virtual to physical address */
  map(SYS_VIRCOPY, do_vircopy); 	/* use pure virtual addressing */
  map(SYS_PHYSCOPY, do_physcopy); 	/* use physical addressing */
  map(SYS_VIRVCOPY, do_virvcopy);	/* vector with copy requests */
  map(SYS_PHYSVCOPY, do_physvcopy);	/* vector with copy requests */

  /* Clock functionality. */
  map(SYS_TIMES, do_times);		/* get uptime and process times */
  map(SYS_SETALARM, do_setalarm);	/* schedule a synchronous alarm */

  /* System control. */
  map(SYS_ABORT, do_abort);		/* abort MINIX */
  map(SYS_GETINFO, do_getinfo); 	/* request system information */ 
}

/*===========================================================================*
 *				get_priv				     *
 *===========================================================================*/
// 为进程分配一个特权级结构体, 即 从系统特权级表中分配一项, 
// 普通用户进程共享同一个特权级结构体
PUBLIC int get_priv(rc, proc_type)
register struct proc *rc;		/* new (child) process pointer */
int proc_type;				/* system or user process flag */
{
/* Get a privilege structure. All user processes share the same privilege 
 * structure. System processes get their own privilege structure. 
 */
/*
 * 获取一个优先级结构体. 所有的用户进程共享同一个优先级结构. 系统进程独享
 * 一个优先级结构体.
 */
  register struct priv *sp;			/* privilege structure */

  // 系统进程
  if (proc_type == SYS_PROC) {			/* find a new slot */
	  // 从系统特权级表中找一个空闲项
      for (sp = BEG_PRIV_ADDR; sp < END_PRIV_ADDR; ++sp) 
	// 如果某一项未与某一进程关联,
	// 并且该项不是普通用户进程共享的特权级结构体, 说明找到一个空闲项.
          if (sp->s_proc_nr == NONE && sp->s_id != USER_PRIV_ID) break;	
      // 如果系统特权级表中没有空闲项剩余, 则返回错误.
      if (sp->s_proc_nr != NONE) return(ENOSPC);
      // 执行到这里说明找到一个空闲项, 在系统特权级表中, 在进程与特权级
      // 结构中更新相应的字段.
      rc->p_priv = sp;				/* assign new slot */
      rc->p_priv->s_proc_nr = proc_nr(rc);	/* set association */
      rc->p_priv->s_flags = SYS_PROC;		/* mark as privileged */
  } else {
	  // 没有特权的普通用户进程共享同一个特权级结构体.
      rc->p_priv = &priv[USER_PRIV_ID];		/* use shared slot */
      rc->p_priv->s_proc_nr = INIT_PROC_NR;	/* set association */
      rc->p_priv->s_flags = 0;			/* no initial flags */
  }
  return(OK);
}

/*===========================================================================*
 *				get_randomness				     *
 *===========================================================================*/
PUBLIC void get_randomness(source)
int source;
{
/* On machines with the RDTSC (cycle counter read instruction - pentium
 * and up), use that for high-resolution raw entropy gathering. Otherwise,
 * use the realtime clock (tick resolution).
 *
 * Unfortunately this test is run-time - we don't want to bother with
 * compiling different kernels for different machines.
 *
 * On machines without RDTSC, we use read_clock().
 */
/*
 * 在拥有 RDTSC (周期计数器读指令 - 奔腾以上的 CPU)的机器上, 使用 RDTSC
 * 可以用来高分辨率原始熵收集. 否则, 使用实时钟(滴答分辨率).
 *
 * 不幸的是这个检查在运行时刻进行的 - 我们不想在不同的机器上编译不同的
 * 内核.
 *
 * 在没有 RDTSC 的机器上, 我们使用 read_clock().
 */
  int r_next;
  unsigned long tsc_high, tsc_low;

  source %= RANDOM_SOURCES;
  r_next= krandom.bin[source].r_next;
  // 如果 CPU 的版本高于 486, 则读取 CPU 的周期计数器值, 并
  // 存在适当位置, 否则读取 8253 计时芯片的计数值.
  if (machine.processor > 486) {
	// _read_tsc
      read_tsc(&tsc_high, &tsc_low);
      krandom.bin[source].r_buf[r_next] = tsc_low;
  } else {
      krandom.bin[source].r_buf[r_next] = read_clock();
  }
  if (krandom.bin[source].r_size < RANDOM_ELEMENTS) {
  	krandom.bin[source].r_size ++;
  }
  krandom.bin[source].r_next = (r_next + 1 ) % RANDOM_ELEMENTS;
}

/*===========================================================================*
 *				send_sig				     *
 *===========================================================================*/
PUBLIC void send_sig(proc_nr, sig_nr)
int proc_nr;			/* system process to be signalled */
int sig_nr;			/* signal to be sent, 1 to _NSIG */
{
/* Notify a system process about a signal. This is straightforward. Simply
 * set the signal that is to be delivered in the pending signals map and 
 * send a notification with source SYSTEM.
 */ 
/*
 * 通知系统进程收到了一个信号. 这很直接了当. 发送信号仅仅是将进程的未决信号
 * 位图中的相应位置位, 然后再向进程发送一个通知.
 */
  register struct proc *rp;

  rp = proc_addr(proc_nr);
  sigaddset(&priv(rp)->s_sig_pending, sig_nr); // sigaddset() ???
  // 由 SYSTEM  给 proc_nr 发送通知.
  lock_notify(SYSTEM, proc_nr); 
}

/*===========================================================================*
 *				cause_sig				     *
 *===========================================================================*/
PUBLIC void cause_sig(proc_nr, sig_nr)
int proc_nr;			/* process to be signalled */
int sig_nr;			/* signal to be sent, 1 to _NSIG */
{
/* A system process wants to send a signal to a process.  Examples are:
 *  - HARDWARE wanting to cause a SIGSEGV after a CPU exception
 *  - TTY wanting to cause SIGINT upon getting a DEL
 *  - FS wanting to cause SIGPIPE for a broken pipe 
 * Signals are handled by sending a message to PM.  This function handles the 
 * signals and makes sure the PM gets them by sending a notification. The 
 * process being signaled is blocked while PM has not finished all signals 
 * for it. 
 * Race conditions between calls to this function and the system calls that
 * process pending kernel signals cannot exist. Signal related functions are
 * only called when a user process causes a CPU exception and from the kernel 
 * process level, which runs to completion.
 */
/*
 * 一个系统进程想要给一个进程发送信号. 例如:
 *   - 发生一个 CPU 异常时, HARDWARE 想要产生一个 SIGSEGV 信号 
 *   - 获得一个 DEL 时, TTY 想要产生一个 SIGINT
 *   - FS 想要为一个破碎的管道产生一个 SIGPIPE 信号 
 * 通过向 PM(process manager? --wzh) 发送消息来处理信号. 该函数处理信号并
 * 确保通过发送通知可以让 PM 得到它们. 当 PM 还没有为进程处理完所有的信号
 * 之前, 该进程阻塞. 
 * 该函数的调用与负责处理未决的系统信号的系统调用之间不能存在竞争条件.
 * 只有当一个用户进程导致一个 CPU 异常时才会调用与信号相关的函数, 这些
 * 函数从内核进程层次调用, 内核进程会完成它们
 */
  register struct proc *rp;

  /* Check if the signal is already pending. Process it otherwise. */
  rp = proc_addr(proc_nr);
  // 如果要发送的信号不是进程的未决信号, 就将其加入到目标进程的未决信号中.
  if (! sigismember(&rp->p_pending, sig_nr)) {
      sigaddset(&rp->p_pending, sig_nr); // sigaddset() ???
      // 如果没有新的内核信号到达
      if (! (rp->p_rts_flags & SIGNALED)) {		/* other pending */
	// "rp->p_rts_flags == 0" means what???
	// lock_dequeue(): 将指定进程从调度队列中移除, 在移除过程中需要加锁.
          if (rp->p_rts_flags == 0) lock_dequeue(rp);	/* make not ready */
          rp->p_rts_flags |= SIGNALED | SIG_PENDING;	/* update flags */
	  // 给进程管理任务发送信号 
          send_sig(PM_PROC_NR, SIGKSIG);
      }
  }
}

/*===========================================================================*
 *				umap_local				     *
 *===========================================================================*/
PUBLIC phys_bytes umap_local(rp, seg, vir_addr, bytes)
register struct proc *rp;	/* pointer to proc table entry for process */
int seg;			/* T, D, or S segment */
vir_bytes vir_addr;		/* virtual address in bytes within the seg */
// bytes 参数用于检查地址的有效性
vir_bytes bytes;		/* # of bytes to be copied */
{
/* Calculate the physical memory address for a given virtual address. */
/* 根据给定的虚拟地址计算物理地址. */
  vir_clicks vc;		/* the virtual address in clicks */
  phys_bytes pa;		/* intermediate variables as phys_bytes */
  phys_bytes seg_base;

  /* If 'seg' is D it could really be S and vice versa.  T really means T.
   * If the virtual address falls in the gap,  it causes a problem. On the
   * 8088 it is probably a legal stack reference, since "stackfaults" are
   * not detected by the hardware.  On 8088s, the gap is called S and
   * accepted, but on other machines it is called D and rejected.
   * The Atari ST behaves like the 8088 in this respect.
   */
  /*
   * 如果 'seg' 是 D, 那么等价于 S, 反之亦然. 而  T 就是 T. 如果虚拟地址
   * 落在了间隙中, 这将造成问题. 在 8088 中, 这可能是一个合法的栈引用, 
   * 因为 "stackfaults" 不由硬件检测. 在 8088 系统芯片中, 间隙称为 S, 
   * 并且是可接受的, 但是在其他机器上称为 D, 并且是禁止的. 在这一方面,
   * Atari ST 的行为类似 8088.
   */

  if (bytes <= 0) return( (phys_bytes) 0);
  if (vir_addr + bytes <= vir_addr) return 0;	/* overflow */
  vc = (vir_addr + bytes - 1) >> CLICK_SHIFT;	/* last click of data */

  if (seg != T)
	seg = (vc < rp->p_memmap[D].mem_vir + rp->p_memmap[D].mem_len ? D : S);

  if ((vir_addr>>CLICK_SHIFT) >= rp->p_memmap[seg].mem_vir + 
  	rp->p_memmap[seg].mem_len) return( (phys_bytes) 0 );

  if (vc >= rp->p_memmap[seg].mem_vir + 
  	rp->p_memmap[seg].mem_len) return( (phys_bytes) 0 );

  seg_base = (phys_bytes) rp->p_memmap[seg].mem_phys;
  seg_base = seg_base << CLICK_SHIFT;	/* segment origin in bytes */
  pa = (phys_bytes) vir_addr;
  // 将 pa 换算成段内偏移地址.
  pa -= rp->p_memmap[seg].mem_vir << CLICK_SHIFT;
  return(seg_base + pa);
}

/*===========================================================================*
 *				umap_remote				     *
 *===========================================================================*/
PUBLIC phys_bytes umap_remote(rp, seg, vir_addr, bytes)
register struct proc *rp;	/* pointer to proc table entry for process */
int seg;			/* index of remote segment */
vir_bytes vir_addr;		/* virtual address in bytes within the seg */
vir_bytes bytes;		/* # of bytes to be copied */
{
/* Calculate the physical memory address for a given virtual address. */
/* 根据给定的虚拟地址计算物理地址 */
  struct far_mem *fm;

  if (bytes <= 0) return( (phys_bytes) 0);
  if (seg < 0 || seg >= NR_REMOTE_SEGS) return( (phys_bytes) 0);

  fm = &rp->p_priv->s_farmem[seg];
  if (! fm->in_use) return( (phys_bytes) 0);
  if (vir_addr + bytes > fm->mem_len) return( (phys_bytes) 0);

  return(fm->mem_phys + (phys_bytes) vir_addr); 
}

/*===========================================================================*
 *				umap_bios				     *
 *===========================================================================*/
PUBLIC phys_bytes umap_bios(rp, vir_addr, bytes)
register struct proc *rp;	/* pointer to proc table entry for process */
vir_bytes vir_addr;		/* virtual address in BIOS segment */
vir_bytes bytes;		/* # of bytes to be copied */
{
/* Calculate the physical memory address at the BIOS. Note: currently, BIOS
 * address zero (the first BIOS interrupt vector) is not considered as an 
 * error here, but since the physical address will be zero as well, the 
 * calling function will think an error occurred. This is not a problem,
 * since no one uses the first BIOS interrupt vector.  
 */
/*
 * 计算位于 BIOS 内存段的物理地址. 注意: 就目前来说, BIOS 地址 0 不被当
 * 作一个错误, 因为物理地址也是 0, 但是主调函数会认为发生了一个错误. 但
 * 这并不是一个问题, 因为没有人会用 BIOS 的第一个中断向量号.
 */

  /* Check all acceptable ranges. */
  if (vir_addr >= BIOS_MEM_BEGIN && vir_addr + bytes <= BIOS_MEM_END)
  	return (phys_bytes) vir_addr;
  else if (vir_addr >= BASE_MEM_TOP && vir_addr + bytes <= UPPER_MEM_END)
  	return (phys_bytes) vir_addr;
  kprintf("Warning, error in umap_bios, virtual address 0x%x\n", vir_addr);
  return 0;
}

/*===========================================================================*
 *				virtual_copy				     *
 *===========================================================================*/
PUBLIC int virtual_copy(src_addr, dst_addr, bytes)
struct vir_addr *src_addr;	/* source virtual address */
struct vir_addr *dst_addr;	/* destination virtual address */
vir_bytes bytes;		/* # of bytes to copy  */
{
/* Copy bytes from virtual address src_addr to virtual address dst_addr. 
 * Virtual addresses can be in ABS, LOCAL_SEG, REMOTE_SEG, or BIOS_SEG.
 */
/*
 * 从虚拟地址 src_addr 复制字节到虚拟地址 dst_addr. 虚拟地址可以在 ABS(???),
 * LOCAL_SEG, REMOTE_SEG, 或 BIOS_SEG.
 */
  struct vir_addr *vir_addr[2];	/* virtual source and destination address */
  phys_bytes phys_addr[2];	/* absolute source and destination */ 
  int seg_index;
  int i;

  /* Check copy count. */
  if (bytes <= 0) return(EDOM);

  /* Do some more checks and map virtual addresses to physical addresses. */
  vir_addr[_SRC_] = src_addr;
  vir_addr[_DST_] = dst_addr;
  for (i=_SRC_; i<=_DST_; i++) {

      /* Get physical address. */
      switch((vir_addr[i]->segment & SEGMENT_TYPE)) {
      case LOCAL_SEG:
          seg_index = vir_addr[i]->segment & SEGMENT_INDEX;
          phys_addr[i] = umap_local( proc_addr(vir_addr[i]->proc_nr), 
              seg_index, vir_addr[i]->offset, bytes );
          break;
      case REMOTE_SEG:
          seg_index = vir_addr[i]->segment & SEGMENT_INDEX;
          phys_addr[i] = umap_remote( proc_addr(vir_addr[i]->proc_nr), 
              seg_index, vir_addr[i]->offset, bytes );
          break;
      case BIOS_SEG:
          phys_addr[i] = umap_bios( proc_addr(vir_addr[i]->proc_nr),
              vir_addr[i]->offset, bytes );
          break;
      case PHYS_SEG:
          phys_addr[i] = vir_addr[i]->offset;
          break;
      default:
          return(EINVAL);
      }

      /* Check if mapping succeeded. */
      if (phys_addr[i] <= 0 && vir_addr[i]->segment != PHYS_SEG) 
          return(EFAULT);
  }

  /* Now copy bytes between physical addresseses. */
  // _phys_copy
  phys_copy(phys_addr[_SRC_], phys_addr[_DST_], (phys_bytes) bytes);
  return(OK);
}

