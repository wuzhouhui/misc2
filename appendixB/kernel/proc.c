/* This file contains essentially all of the process and message handling.
 * Together with "mpx.s" it forms the lowest layer of the MINIX kernel.
 * There is one entry point from the outside:
 *
 *   sys_call: 	      a system call, i.e., the kernel is trapped with an INT
 *
 * As well as several entry points used from the interrupt and task level:
 *
 *   lock_notify:     notify a process of a system event
 *   lock_send:	      send a message to a process
 *   lock_enqueue:    put a process on one of the scheduling queues 
 *   lock_dequeue:    remove a process from the scheduling queues
 *
 * Changes:
 *   Aug 19, 2005     rewrote scheduling code  (Jorrit N. Herder)
 *   Jul 25, 2005     rewrote system call handling  (Jorrit N. Herder)
 *   May 26, 2005     rewrote message passing functions  (Jorrit N. Herder)
 *   May 24, 2005     new notification system call  (Jorrit N. Herder)
 *   Oct 28, 2004     nonblocking send and receive calls  (Jorrit N. Herder)
 *
 * The code here is critical to make everything work and is important for the
 * overall performance of the system. A large fraction of the code deals with
 * list manipulation. To make this both easy to understand and fast to execute 
 * pointer pointers are used throughout the code. Pointer pointers prevent
 * exceptions for the head or tail of a linked list. 
 *
 *  node_t *queue, *new_node;	// assume these as global variables
 *  node_t **xpp = &queue; 	// get pointer pointer to head of queue 
 *  while (*xpp != NULL) 	// find last pointer of the linked list
 *      xpp = &(*xpp)->next;	// get pointer to next pointer 
 *  *xpp = new_node;		// now replace the end (the NULL pointer) 
 *  new_node->next = NULL;	// and mark the new end of the list
 * 
 * For example, when adding a new node to the end of the list, one normally 
 * makes an exception for an empty list and looks up the end of the list for 
 * nonempty lists. As shown above, this is not required with pointer pointers.
 */
/*
 * 这个文件含有所有进程与消息处理有关的代码. 与 "mpx.s" 一起组成了 MINIX
 * 内核的底层代码. 这里只有一个从外部进入的入口:
 *	sys_call:	一个系统调用, 换句话说, 利用 INT 来陷入到内核中.
 * 从中断任务层也有几个入口点:
 *	lock_notify:	通知进程发生了一个系统事件;
 *	lock_send:	向一个进程发送消息;
 *	lock_enqueue:	将一个进程插入到一个调度队列中;
 *	lock_dequeue:	从调度队列中移除一个进程.
 *
 * 改变:
 *	...
 * 
 * 这里的代码对系统运行和整体性能至关重要. 相当一部分的代码与链表操作有关.
 * 为了使链表操作更加容易理解与高效, 在代码中大量使用了二级指针. 二级指针
 * 避免了链表头或链表尾的异常情况.
 *
 * node_t *queue, *new_node;	// 假定变量是全局的
 * node_t **xpp = &queue;	// 获取队首的二级指针
 * while (*xpp != NULL)		// 找到链表中的最后一个元素
 *	xpp = &(*xpp)->next;	// 获取下一个指针的指针.
 * *xpp = new_node;		// 现在替换队尾
 * new_node->next = NULL;	// 标记链表的结尾.
 *
 * 例如, 当往一个链表末尾添加一个新结点时, 如果链表为空, 则会导致一个异常,
 * 然后搜索链表, 找到最后一个元素. 就像上面所展现的那样, 这对于二级指针来
 * 说不是必要的.
 */

#include <minix/com.h>
#include <minix/callnr.h>
#include "kernel.h"
#include "proc.h"

/* Scheduling and message passing functions. The functions are available to 
 * other parts of the kernel through lock_...(). The lock temporarily disables 
 * interrupts to prevent race conditions. 
 */
/*
 * 调度与消息传递函数. 这些函数对于内核中的其他函数也是可用的. 对中断的
 * 临时屏蔽可以避免竞争条件.
 */
FORWARD _PROTOTYPE( int mini_send, (struct proc *caller_ptr, int dst,
		message *m_ptr, unsigned flags) );
FORWARD _PROTOTYPE( int mini_receive, (struct proc *caller_ptr, int src,
		message *m_ptr, unsigned flags) );
FORWARD _PROTOTYPE( int mini_notify, (struct proc *caller_ptr, int dst) );

FORWARD _PROTOTYPE( void enqueue, (struct proc *rp) );
FORWARD _PROTOTYPE( void dequeue, (struct proc *rp) );
FORWARD _PROTOTYPE( void sched, (struct proc *rp, int *queue, int *front) );
FORWARD _PROTOTYPE( void pick_proc, (void) );

// 构建一个消息
#define BuildMess(m_ptr, src, dst_ptr) \
	// 消息的发送者
	(m_ptr)->m_source = (src); 					\
	// 消息的类型, 通知
	(m_ptr)->m_type = NOTIFY_FROM(src);				\
	// 消息发送的时间
	(m_ptr)->NOTIFY_TIMESTAMP = get_uptime();			\
	switch (src) {							\
	case HARDWARE:							\
		// 如果发送者是 HARDWARE, 将通知的参数设置为接收进程的
		// 未决中断, 同时复位接收进程的未决中断
		(m_ptr)->NOTIFY_ARG = priv(dst_ptr)->s_int_pending;	\
		priv(dst_ptr)->s_int_pending = 0;			\
		break;							\
	case SYSTEM:							\
		// 如果发送者是 SYSTEM, 将通知的参数设置为接收进程的
		// 未决信号, 同时复位接收进程的未决信号
		(m_ptr)->NOTIFY_ARG = priv(dst_ptr)->s_sig_pending;	\
		priv(dst_ptr)->s_sig_pending = 0;			\
		break;							\
	}

#define CopyMess(s,sp,sm,dp,dm) \
	// _cp_mess
	cp_mess(s, (sp)->p_memmap[D].mem_phys,	\
		 (vir_bytes)sm, (dp)->p_memmap[D].mem_phys, (vir_bytes)dm)

/*===========================================================================*
 *				sys_call				     * 
 *===========================================================================*/
PUBLIC int sys_call(call_nr, src_dst, m_ptr)
int call_nr;			/* system call number and flags */
int src_dst;			/* src to receive from or dst to send to */
message *m_ptr;			/* pointer to message in the caller's space */
{
/* System calls are done by trapping to the kernel with an INT instruction.
 * The trap is caught and sys_call() is called to send or receive a message
 * (or both). The caller is always given by 'proc_ptr'.
 */
/*
 * 系统调用通过一个中断指针并陷入内核中来完成. 陷阱被捕获, 然后调用
 * sys_call() 来发送或接收一个消息. 主调进程总是通过 'proc_ptr' 指明.
 */
  register struct proc *caller_ptr = proc_ptr;	/* get pointer to caller */
  int function = call_nr & SYSCALL_FUNC;	/* get system call function */
  unsigned flags = call_nr & SYSCALL_FLAGS;	/* get flags */
  int mask_entry;				/* bit to check in send mask */
  int result;					/* the system call's result */
  vir_clicks vlo, vhi;		/* virtual clicks containing message to send */

  /* Check if the process has privileges for the requested call. Calls to the 
   * kernel may only be SENDREC, because tasks always reply and may not block 
   * if the caller doesn't do receive(). 
   */
  /*
   * 检查进程对该请求是否有特权. 调用内核的可能只有 SENDREC, 因为任务总是
   * 依赖于并且不会阻塞, 如果主调进程没有调用 receive() 的话.
   */
  // 如果主调进程没有权限调用某个系统调用, 或者 
  // 发送进程是内核任务并且系统调用不是发送或接收, 并且 
  // 系统调用不是阻塞发送, 则报错.
  if (! (priv(caller_ptr)->s_trap_mask & (1 << function)) || 
          (iskerneln(src_dst) && function != SENDREC
           && function != RECEIVE)) { 
      kprintf("sys_call: trap %d not allowed, caller %d, src_dst %d\n", 
          function, proc_nr(caller_ptr), src_dst);
      return(ECALLDENIED);		/* trap denied by mask or kernel */
  }
  
  /* Require a valid source and/ or destination process, unless echoing. */
  /* 要求一个有效的发送与接收进程, 除非是回射 */
  // 如果进程号无效, 并且进程号不是任意进程, 并且系统调用不是回射, 
  // 则出错返回.
  if (! (isokprocn(src_dst) || src_dst == ANY || function == ECHO)) { 
      kprintf("sys_call: invalid src_dst, src_dst %d, caller %d\n", 
          src_dst, proc_nr(caller_ptr));
      return(EBADSRCDST);		/* invalid process number */
  }

  /* If the call involves a message buffer, i.e., for SEND, RECEIVE, SENDREC, 
   * or ECHO, check the message pointer. This check allows a message to be 
   * anywhere in data or stack or gap. It will have to be made more elaborate 
   * for machines which don't have the gap mapped. 
   */
  /*
   * 如果调用牵涉到消息缓冲区, 也就是说调用是 SEND, RECEIVE, SENDREC,
   * 或 ECHO, 检查消息指针. 这个检查允许消息在数据区, 栈区或间隙的任何地方.
   * 对于那些没有间隙映射的机器来说, 这个检查要做得更加的精细.
   */
  if (function & CHECK_PTR) {	
	  // vlo: 消息缓冲区的开始地址, 单位 click
      vlo = (vir_bytes) m_ptr >> CLICK_SHIFT;		
      // vli: 消息缓冲区的结束地址, 单位 click
      vhi = ((vir_bytes) m_ptr + MESS_SIZE - 1) >> CLICK_SHIFT;
      // 如果消息缓冲区的起始地址小于主调进程的数据区起始地址,
      // 或者 消息缓冲区的起始地址大于结束地址,
      // 或者 消息缓冲区的结束地址大于主调进程的堆栈区结束地址,
      // 出错返回.
      if (vlo < caller_ptr->p_memmap[D].mem_vir || vlo > vhi ||
              vhi >= caller_ptr->p_memmap[S].mem_vir + 
              caller_ptr->p_memmap[S].mem_len) {
          kprintf("sys_call: invalid message pointer, trap %d, caller %d\n",
          	function, proc_nr(caller_ptr));
          return(EFAULT); 		/* invalid message pointer */
      }
  }

  /* If the call is to send to a process, i.e., for SEND, SENDREC or NOTIFY,
   * verify that the caller is allowed to send to the given destination and
   * that the destination is still alive. 
   */
  /*
   * 如果调用是给一个进程发送数据, 也就是说 SEND, SENDREC, NOTIFY,
   * 验证主调进程是否允许向指针的目标进程发送消息, 同时要也验证目标进程
   * 是否仍然是活动的.
   */
  // 如果调用需要检查目标进程. 则检查主调进程允许向 src_dst 通信, 以及 
  // src_dst 是否仍然是活动的.
  if (function & CHECK_DST) {	
	  // 如果主调进程无法向 src_dst 通信, 则出错返回.
      if (! get_sys_bit(priv(caller_ptr)->s_ipc_to, nr_to_id(src_dst))) {
          kprintf("sys_call: ipc mask denied %d sending to %d\n",
          	proc_nr(caller_ptr), src_dst);
          return(ECALLDENIED);		/* call denied by ipc mask */
      }

      // 如果源或目标是空的, 并且现在正在关机, 则出错返回.
      if (isemptyn(src_dst) && !shutdown_started) {
          kprintf("sys_call: dead dest; %d, %d, %d\n", 
              function, proc_nr(caller_ptr), src_dst);
          return(EDEADDST); 		/* cannot send to the dead */
      }
  }

  /* Now check if the call is known and try to perform the request. The only
   * system calls that exist in MINIX are sending and receiving messages.
   *   - SENDREC: combines SEND and RECEIVE in a single system call
   *   - SEND:    sender blocks until its message has been delivered
   *   - RECEIVE: receiver blocks until an acceptable message has arrived
   *   - NOTIFY:  nonblocking call; deliver notification or mark pending
   *   - ECHO:    nonblocking call; directly echo back the message 
   */
  /*
   * 现在检查调用是否是已知的, 然后尝试执行请求. 在 MINIX 中存在的系统
   * 调用只有发送与接收消息.
   *	- SENDREC: 把 SEND 与 RECEIVE 联合到一个系统调用中.
   *	- SEND:    发送进程阻塞到消息被递送为止.
   *	- RECEIVE: 接收进程阻塞到一个可接收的消息到达为止.
   *	- NOTIFY:  非阻塞调用; 递送一个通知或标记一个未决通知.
   *	- ECHO:    非阻塞调用; 直接回射到消息本身???
   */
  switch(function) {
  case SENDREC:
      /* A flag is set so that notifications cannot interrupt SENDREC. */
	/* 设置一个标记, 这样通知就不会中断 SENDREC */
      priv(caller_ptr)->s_flags |= SENDREC_BUSY;
      /* fall through */
      /* 继续往下 */
  case SEND:			
      // 向目标进程发送消息, 有可能阻塞.
      result = mini_send(caller_ptr, src_dst, m_ptr, flags);
      if (function == SEND || result != OK) {	
          break;				/* done, or SEND failed */
      }						/* fall through for SENDREC */
  case RECEIVE:			
      if (function == RECEIVE)
          priv(caller_ptr)->s_flags &= ~SENDREC_BUSY;
      // 从源接收一个消息, 有可能阻塞
      result = mini_receive(caller_ptr, src_dst, m_ptr, flags);
      break;
  case NOTIFY:
      // 向一个进程发送通知.
      result = mini_notify(caller_ptr, src_dst);
      break;
  case ECHO:
      // 将消息发送给自己
      CopyMess(caller_ptr->p_nr, caller_ptr, m_ptr, caller_ptr, m_ptr);
      result = OK;
      break;
  default:
      result = EBADCALL;			/* illegal system call */
  }

  /* Now, return the result of the system call to the caller. */
  return(result);
}

/*===========================================================================*
 *				mini_send				     * 
 *===========================================================================*/
PRIVATE int mini_send(caller_ptr, dst, m_ptr, flags)
register struct proc *caller_ptr;	/* who is trying to send a message? */
int dst;				/* to whom is message being sent? */
message *m_ptr;				/* pointer to message buffer */
unsigned flags;				/* system call flags */
{
/* Send a message from 'caller_ptr' to 'dst'. If 'dst' is blocked waiting
 * for this message, copy the message to it and unblock 'dst'. If 'dst' is
 * not waiting at all, or is waiting for another source, queue 'caller_ptr'.
 */
/*
 * 从 'caller_ptr' 向 'dst' 发送一个消息. 如果 'dst' 正阻塞在等待该消息上,
 * 把消息复制给它, 然后解除 'dst' 的阻塞. 如果 'dst' 根本没在等待, 或者 
 * 在等待其他的消息, 那就对 'caller_ptr' 排队.
 */
  register struct proc *dst_ptr = proc_addr(dst);
  register struct proc **xpp;
  register struct proc *xp;

  /* Check for deadlock by 'caller_ptr' and 'dst' sending to each other. */
  /* 检查是否有因 'caller_ptr' 与 'dst' 互相发送消息造成的死锁 */
  xp = dst_ptr;
  while (xp->p_rts_flags & SENDING) {		/* check while sending */
  	xp = proc_addr(xp->p_sendto);		/* get xp's destination */
  	if (xp == caller_ptr) return(ELOCKED);	/* deadlock if cyclic */
  }

  /* Check if 'dst' is blocked waiting for this message. The destination's 
   * SENDING flag may be set when its SENDREC call blocked while sending.  
   */
  /*
   * 检查 'dst' 是否在阻塞等待该消息. 目标进程的 SENDING 标志可能被设置,
   * 当 SENDREC 调用阻塞在 SEND 上时.
   */
  if ( (dst_ptr->p_rts_flags & (RECEIVING | SENDING)) == RECEIVING &&
       (dst_ptr->p_getfrom == ANY || dst_ptr->p_getfrom == caller_ptr->p_nr)) {
	/* Destination is indeed waiting for this message. */
	CopyMess(caller_ptr->p_nr, caller_ptr, m_ptr, dst_ptr,
		 dst_ptr->p_messbuf);
	// 如果接收进程解除接收阻塞后, 没有阻塞在其他事件上, 则将其插入到
	// 调度队列.
	if ((dst_ptr->p_rts_flags &= ~RECEIVING) == 0) enqueue(dst_ptr);
  } else if ( ! (flags & NON_BLOCKING)) {
	/* Destination is not waiting.  Block and dequeue caller. */
	/* 目标进程没有在等待, 阻塞发送进程, 并从调度队列中移除 */
	caller_ptr->p_messbuf = m_ptr;
	if (caller_ptr->p_rts_flags == 0) dequeue(caller_ptr);
	caller_ptr->p_rts_flags |= SENDING;
	caller_ptr->p_sendto = dst;

	/* Process is now blocked.  Put in on the destination's queue. */
	/* 发送进程现在已经阻塞, 将其放到目标进程的等待队列中 */
	xpp = &dst_ptr->p_caller_q;		/* find end of list */
	while (*xpp != NIL_PROC) xpp = &(*xpp)->p_q_link;	
	*xpp = caller_ptr;			/* add caller to end */
	caller_ptr->p_q_link = NIL_PROC;	/* mark new end of list */
  } else {
	return(ENOTREADY);
  }
  return(OK);
}

/*===========================================================================*
 *				mini_receive				     * 
 *===========================================================================*/
PRIVATE int mini_receive(caller_ptr, src, m_ptr, flags)
register struct proc *caller_ptr;	/* process trying to get message */
int src;				/* which message source is wanted */
message *m_ptr;				/* pointer to message buffer */
unsigned flags;				/* system call flags */
{
/* A process or task wants to get a message.  If a message is already queued,
 * acquire it and deblock the sender.  If no message from the desired source
 * is available block the caller, unless the flags don't allow blocking.  
 */
/*
 * 一个进程或任务想要接收一个消息. 如果一个消息已经在排队,
 * 就获取该消息并解除发送者的阻塞. 如果在发送者那儿没有消息, 就阻塞接收
 * 进程, 除非设置了非阻塞标志.
 */
  register struct proc **xpp;
  register struct notification **ntf_q_pp;
  message m;
  int bit_nr;
  sys_map_t *map;
  bitchunk_t *chunk;
  int i, src_id, src_proc_nr;

  /* Check to see if a message from desired source is already available.
   * The caller's SENDING flag may be set if SENDREC couldn't send. If it is
   * set, the process should be blocked.
   */
  /*
   * 检查源进程的消息是否是可用的. 主调进程的 SENDING 可能被设置, 如果 
   * SENDREC 阻塞在 SEND 上. 如果该标志被设置了, 则仍旧阻塞.
   */
  // 如果主调进程没有阻塞在发送消息(SENDING) 上.
  if (!(caller_ptr->p_rts_flags & SENDING)) {

    /* Check if there are pending notifications, except for SENDREC. */
	/* 检查是否有未决的通知, 除了 SENDREC. */
    if (! (priv(caller_ptr)->s_flags & SENDREC_BUSY)) {

        map = &priv(caller_ptr)->s_notify_pending;
        for (chunk=&map->chunk[0]; chunk<&map->chunk[NR_SYS_CHUNKS]; chunk++) {

            /* Find a pending notification from the requested source. */ 
            if (! *chunk) continue; 			/* no bits in chunk */
	    // 寻找第一个不为 0 的位.
            for (i=0; ! (*chunk & (1<<i)); ++i) {} 	/* look up the bit */
            src_id = (chunk - &map->chunk[0]) * BITCHUNK_BITS + i;
            if (src_id >= NR_SYS_PROCS) break;		/* out of range */
            src_proc_nr = id_to_nr(src_id);		/* get source proc */
	    // 如果源还没有准备好, 则跳到下一个位图块, 可是照这样
	    // 做的话, 每个位图块程序只检查了第一个非零位, 但是
	    // 同一个位图块中可能有多个非零位.
            if (src!=ANY && src!=src_proc_nr) continue;	/* source not ok */
            *chunk &= ~(1 << i);			/* no longer pending */

            /* Found a suitable source, deliver the notification message. */
	    BuildMess(&m, src_proc_nr, caller_ptr);	/* assemble message */
            CopyMess(src_proc_nr, proc_addr(HARDWARE), &m, caller_ptr, m_ptr);
            return(OK);					/* report success */
        }
    }

    /* Check caller queue. Use pointer pointers to keep code simple. */
    xpp = &caller_ptr->p_caller_q;
    while (*xpp != NIL_PROC) {
        if (src == ANY || src == proc_nr(*xpp)) {
	    /* Found acceptable message. Copy it and update status. */
	    CopyMess((*xpp)->p_nr, *xpp, (*xpp)->p_messbuf, caller_ptr, m_ptr);
            if (((*xpp)->p_rts_flags &= ~SENDING) == 0) enqueue(*xpp);
            *xpp = (*xpp)->p_q_link;		/* remove from queue */
            return(OK);				/* report success */
	}
	xpp = &(*xpp)->p_q_link;		/* proceed to next */
    }
  }

  /* No suitable message is available or the caller couldn't send in SENDREC. 
   * Block the process trying to receive, unless the flags tell otherwise.
   */
  if ( ! (flags & NON_BLOCKING)) {
      caller_ptr->p_getfrom = src;		
      caller_ptr->p_messbuf = m_ptr;
      if (caller_ptr->p_rts_flags == 0) dequeue(caller_ptr);
      caller_ptr->p_rts_flags |= RECEIVING;		
      return(OK);
  } else {
      return(ENOTREADY);
  }
}

/*===========================================================================*
 *				mini_notify				     * 
 *===========================================================================*/
// 给进程发送通知. 如果接收进程没有准备好, 则在接收进程中设置与发送进程相对应的
// 未决通知位, 并返回
PRIVATE int mini_notify(caller_ptr, dst)
register struct proc *caller_ptr;	/* sender of the notification */
// 被通知的进程的进程号
int dst;				/* which process to notify */
{
	// 获取被通知进程的地址
  register struct proc *dst_ptr = proc_addr(dst);
  int src_id;				/* source id for late delivery */
  message m;				/* the notification message */

  /* Check to see if target is blocked waiting for this message. A process 
   * can be both sending and receiving during a SENDREC system call.
   */
  /* 
   * 查看目录进程是否因等待消息而阻塞. 在系统调用 SENDREC 中, 同一个进程可
   * 以同时发送, 接收消息.
   */
  // 如果目标进程正阻塞在接收消息上, 并且不是正在调用 sendrec(),
  // 并且目标进程可以接收任何进程或发送进程发送的消息
  if ((dst_ptr->p_rts_flags & (RECEIVING|SENDING)) == RECEIVING &&
      ! (priv(dst_ptr)->s_flags & SENDREC_BUSY) &&
      (dst_ptr->p_getfrom == ANY || dst_ptr->p_getfrom == caller_ptr->p_nr)) {

      /* Destination is indeed waiting for a message. Assemble a notification 
       * message and deliver it. Copy from pseudo-source HARDWARE, since the
       * message is in the kernel's address space.
       */ 
	/*
	 * 目标进程的确在等待接收一个消息. 组装一个通知消息并递送. 
	 * 消息的内容从伪发送端 HARDWARE 复制而来, 因为消息在内核地址空间.
	 */
	// 根据发送者与接收者组装消息的内容
      BuildMess(&m, proc_nr(caller_ptr), dst_ptr);
      // 将消息复制到目的进程的消息缓冲中
      CopyMess(proc_nr(caller_ptr), proc_addr(HARDWARE), &m, 
          dst_ptr, dst_ptr->p_messbuf);
      // 解除接收进程的阻塞状态
      dst_ptr->p_rts_flags &= ~RECEIVING;	/* deblock destination */
      // 如果接收进程并没有因为接收或发送消息而阻塞, 则将其插入调度队列
      if (dst_ptr->p_rts_flags == 0) enqueue(dst_ptr);
      return(OK);
  } 

  /* Destination is not ready to receive the notification. Add it to the 
   * bit map with pending notifications. Note the indirectness: the system id 
   * instead of the process number is used in the pending bit map.
   */ 
  /*
   * 目标没有准备好接收通知. 将未决的通知加入到位图中. 注意到其中的间接性:
   * 在未决的位图中使用了进程优先级结构的系统标识, 而不是进程号.
   */
  // 获取发送进程的特权级的系统标识
  src_id = priv(caller_ptr)->s_id;
  // 设置接收进程的未决通知位图的相应位
  set_sys_bit(priv(dst_ptr)->s_notify_pending, src_id); 
  return(OK);
}

/*===========================================================================*
 *				lock_notify				     *
 *===========================================================================*/
// 加锁(如果未加锁的话), 再给目标进程发送通知
PUBLIC int lock_notify(src, dst)
	// 旧风格的 C 语言函数定义形式
int src;			/* sender of the notification */
int dst;			/* who is to be notified */
{
/* Safe gateway to mini_notify() for tasks and interrupt handlers. The sender
 * is explicitly given to prevent confusion where the call comes from. MINIX 
 * kernel is not reentrant, which means to interrupts are disabled after 
 * the first kernel entry (hardware interrupt, trap, or exception). Locking
 * is done by temporarily disabling interrupts. 
 */
/*
 * 这个函数是任务和中断服务程序通往 mini_notify() 的安全通道. 为了避免由于
 * 不知道调用的来源而造成的混乱, 发送者需要显式指定. MINIX 内核是不可重入
 * 的, 这意味着在硬件中断, 陷阱或异常处理过程中, 中断是被禁止的. 加锁通过
 * 临时禁止中断来实现.
 */
  int result;

  /* Exception or interrupt occurred, thus already locked. */
  // 如果 k_reenter >= 0, 说明异常或中断已经发生, 已经是锁定状态, 直接向
  // 目标进程发送通知.
  if (k_reenter >= 0) {
      result = mini_notify(proc_addr(src), dst); 
  }

  /* Call from task level, locking is required. */
  /* 从任务层调用, 需要加锁 */
  else {
      lock(0, "notify");
      result = mini_notify(proc_addr(src), dst); 
      unlock(0);
  }
  return(result);
}

/*===========================================================================*
 *				enqueue					     * 
 *===========================================================================*/
/*
 * static void enqueue(struct proc *rp)
 */
// 将进程插入对应的队列, 并从所有的队列中选择一个进程, 作为下一次运行
// 的进程
PRIVATE void enqueue(rp)
register struct proc *rp;	/* this process is now runnable */
{
/* Add 'rp' to one of the queues of runnable processes.  This function is 
 * responsible for inserting a process into one of the scheduling queues. 
 * The mechanism is implemented here.   The actual scheduling policy is
 * defined in sched() and pick_proc().
 */
/*
 * 将 'rp' 添加到某个队列之中, 这些队列是由可运行的进程组成的. 该函数负责
 * 将一个进程插入到一个调度队列中. 真正的调度策略在 sched() 和 pick_proc()
 * 中定义.
 */
  int q;	 				/* scheduling queue to use */
  int front;					/* add to front or back */

  /* Determine where to insert to process. */
  // 确定要将进程插入到哪个调度队列中, 以及是插在队头还是队尾
  sched(rp, &q, &front);

  /* Now add the process to the queue. */
  // 如果调度队列为空, 则直接插入, 因为此时插在队尾与队头是一样的
  if (rdy_head[q] == NIL_PROC) {		/* add to empty queue */
      rdy_head[q] = rdy_tail[q] = rp; 		/* create a new queue */
      rp->p_nextready = NIL_PROC;		/* mark new end */
  } 
  // 插在队头
  else if (front) {				/* add to head of queue */
      rp->p_nextready = rdy_head[q];		/* chain head of queue */
      rdy_head[q] = rp;				/* set new queue head */
  } 
  // 插在队尾
  else {					/* add to tail of queue */
      rdy_tail[q]->p_nextready = rp;		/* chain tail of queue */	
      rdy_tail[q] = rp;				/* set new queue tail */
      rp->p_nextready = NIL_PROC;		/* mark new end */
  }

  /* Now select the next process to run. */
  // 选择一个进程来运行
  pick_proc();			
}

/*===========================================================================*
 *				dequeue					     * 
 *===========================================================================*/
// 将进程从队列中移除, 必要的话, 重新选择一个新进程作为下一次调度的选择.
PRIVATE void dequeue(rp)
register struct proc *rp;	/* this process is no longer runnable */
{
/* A process must be removed from the scheduling queues, for example, because
 * it has blocked.  If the currently active process is removed, a new process
 * is picked to run by calling pick_proc().
 */
/*
 * 有一个进程将被移除出调度队列, 例如它是阻塞的. 如果当前的活动进程被移除,
 * 通过 pick_proc() 挑选一个新进程运行.
 */
  register int q = rp->p_priority;		/* queue to use */
  register struct proc **xpp;			/* iterate over queue */
  register struct proc *prev_xp;

  /* Side-effect for kernel: check if the task's stack still is ok? */
  /* 内核的副作用: 检查任务的栈是否完好 */
  // 如果要被移除的任务是内核任务, 则检查任务的堆栈是否完好, 如果堆
  // 栈警戒字发生了变化, 系统崩溃
  if (iskernelp(rp)) { 				
	if (*priv(rp)->s_stack_guard != STACK_GUARD)
		panic("stack overrun by task", proc_nr(rp));
  }

  /* Now make sure that the process is not in its ready queue. Remove the 
   * process if it is found. A process can be made unready even if it is not 
   * running by being sent a signal that kills it.
   */
  /* 
   * 现在确定进程是否在就绪队列中, 如果在的话就移除. ????
   */
  prev_xp = NIL_PROC;				
  // 遍历进程所在的调度队列, 如果找到了进程, 则将其移除出队列
  for (xpp = &rdy_head[q]; *xpp != NIL_PROC; xpp = &(*xpp)->p_nextready) {

      if (*xpp == rp) {				/* found process to remove */
          *xpp = (*xpp)->p_nextready;		/* replace with next chain */
	  // 如果进程是队列的最后一个元素, 将队列尾指针前移
          if (rp == rdy_tail[q])		/* queue tail removed */
              rdy_tail[q] = prev_xp;		/* set new tail */
	  // 如果进程是当前进程, 或者是下一次要运行的进程, 则重新选择一个
	  // 进程来运行.
	  // rp == proc_ptr ??? 
          if (rp == proc_ptr || rp == next_ptr)	/* active process removed */
              pick_proc();			/* pick new process to run */
          break;
      }
      prev_xp = *xpp;				/* save previous in chain */
  }
}

/*===========================================================================*
 *				sched					     * 
 *===========================================================================*/
// 确定要将进程插入到哪个调度队列中, 以及如何插入
// rp: 被调度的进程
// queue: 队列索引
// front: 非0则插在队头, 0 插在队尾
PRIVATE void sched(rp, queue, front)
register struct proc *rp;			/* process to be scheduled */
int *queue;					/* return: queue to use */
int *front;					/* return: front or back */
{
/* This function determines the scheduling policy.  It is called whenever a
 * process must be added to one of the scheduling queues to decide where to
 * insert it.  As a side-effect the process' priority may be updated.  
 */
/*
 * 该函数确定调度策略. 当有一个进程要被插入到调度队列时, 这个函数被调用.
 * 函数调用的副作用是进程(rp)的优先级会被修改.
 */
  static struct proc *prev_ptr = NIL_PROC;	/* previous without time */
  // 将被插入的进程是否还有剩余时间片.
  int time_left = (rp->p_ticks_left > 0);	/* quantum fully consumed */
  int penalty = 0;				/* change in priority */

  /* Check whether the process has time left. Otherwise give a new quantum 
   * and possibly raise the priority.  Processes using multiple quantums 
   * in a row get a lower priority to catch infinite loops in high priority
   * processes (system servers and drivers). 
   */
  /*
   * 检查将被插入的进程是否还有剩余时间片. 如果没有的话, 重新给剩余时间
   * 片赋值, 有可能的话提升进程的优先级. ???
   */
  // 如果进程的剩余运行时间片已用完, 则重新给它赋值.
  // 如果进程上一次刚执行过, 则加以惩罚, 否则的话给予优惠
  if ( ! time_left) {				/* quantum consumed ? */
	// 重新给运行时间片赋值
      rp->p_ticks_left = rp->p_quantum_size; 	/* give new quantum */
      // 如果进程上一次刚执行过, 加以惩罚, 否则的话, 给予优惠
      if (prev_ptr == rp) penalty ++;		/* catch infinite loops */
      else penalty --; 				/* give slow way back */
      prev_ptr = rp;				/* store ptr for next */
  }

  /* Determine the new priority of this process. The bounds are determined
   * by IDLE's queue and the maximum priority of this process. Kernel tasks 
   * and the idle process are never changed in priority.
   */
  /*
   * 确定进程的新优先级. 边界由 IDLE 的队列与进程的最大优先级确定. 内核任务
   * 和空闲进程从不改变优先级.
   */
  // 计算进程的新优先级. 如果进程被惩罚或给予优惠, 并且不是内核进程,
  // 则重新计算优先的优先级. 如果新优先级超过了进程的最高优先级或比最低
  // 优先级还要低, 则赋予边界值. 
  if (penalty != 0 && ! iskernelp(rp)) {
      rp->p_priority += penalty;		/* update with penalty */
      if (rp->p_priority < rp->p_max_priority)  /* check upper bound */ 
          rp->p_priority=rp->p_max_priority;
      else if (rp->p_priority > IDLE_Q-1)   	/* check lower bound */
      	  rp->p_priority = IDLE_Q-1;
  }

  /* If there is time left, the process is added to the front of its queue, 
   * so that it can immediately run. The queue to use simply is always the
   * process' current priority. 
   */
  /*
   * 如果进程在调度之间还有剩余时间片, 则插入在队列头, 因此它可以马上运行.
   * 队列部是使用与进程优先级对应的队列.
   */
  // 每个优先级都有相对应的队列
  *queue = rp->p_priority;
  *front = time_left;
}

/*===========================================================================*
 *				pick_proc				     * 
 *===========================================================================*/
PRIVATE void pick_proc()
{
/* Decide who to run now.  A new process is selected by setting 'next_ptr'.
 * When a billable process is selected, record it in 'bill_ptr', so that the 
 * clock task can tell who to bill for system time.
 */
/*
 * 选择一个进程来运行. 通过设置 'next_ptr' 来选择一个进程. 当一个可记帐
 * 的进程被选择时, 将它记录到 bill_ptr 中, 于是时钟任务可以知道 ???.
 */
  register struct proc *rp;			/* process to run */
  int q;					/* iterate over queues */

  /* Check each of the scheduling queues for ready processes. The number of
   * queues is defined in proc.h, and priorities are set in the image table.
   * The lowest queue contains IDLE, which is always ready.
   */
  /*
   * 为就绪的进程检查每一个调度队列. 队列的数量在 proc.h 中定义, 它们的
   * 优先级在一个镜像表格中记录. 优先级最低的队列含有 IDLE, 它总是就绪的.
   */
  // 从优先级最高的队列开始, 如果找到一个不为空的队列, 则取队头元素作为
  // 下一个运行的进程. 如果进程是可记帐的, 则将其赋给 bill_ptr.
  for (q=0; q < NR_SCHED_QUEUES; q++) {	
      if ( (rp = rdy_head[q]) != NIL_PROC) {
          next_ptr = rp;			/* run process 'rp' next */
          if (priv(rp)->s_flags & BILLABLE)	 	
              bill_ptr = rp;			/* bill for system time */
          return;				 
      }
  }
}

/*===========================================================================*
 *				lock_send				     *
 *===========================================================================*/
// 加锁, 然后再发送消息, 再解锁, 这是任务通往 mini_send() 网关.
PUBLIC int lock_send(dst, m_ptr)
int dst;			/* to whom is message being sent? */
message *m_ptr;			/* pointer to message buffer */
{
/* Safe gateway to mini_send() for tasks. */
  int result;
  lock(2, "send");
  result = mini_send(proc_ptr, dst, m_ptr, NON_BLOCKING);
  unlock(2);
  return(result);
}

/*===========================================================================*
 *				lock_enqueue				     *
 *===========================================================================*/
// 加锁, 再将进程添加至调度队列.
PUBLIC void lock_enqueue(rp)
struct proc *rp;		/* this process is now runnable */
{
/* Safe gateway to enqueue() for tasks. */
  lock(3, "enqueue");
  enqueue(rp);
  unlock(3);
}

/*===========================================================================*
 *				lock_dequeue				     *
 *===========================================================================*/
// 将指定进程从调度队列中移除, 在移除过程中需要加锁.
PUBLIC void lock_dequeue(rp)
struct proc *rp;		/* this process is no longer runnable */
{
/* Safe gateway to dequeue() for tasks. */
  lock(4, "dequeue");
  dequeue(rp);
  unlock(4);
}

