/* The kernel call implemented in this file:
 *   m_type:	SYS_EXIT
 *
 * The parameters for this kernel call are:
 *    m1_i1:	PR_PROC_NR		(slot number of exiting process)
 */
/*
 * 该文件实现的系统调用:
 *	m_type:	SYS_EXIT
 *
 * 该系统调用的参数包括:
 *	m1_i1:	PR_PROC_NR		(正在退出的进程在进程表中的槽号)
 */

#include "../system.h"

#if USE_EXIT

FORWARD _PROTOTYPE( void clear_proc, (register struct proc *rc));

/*===========================================================================*
 *				do_exit					     *
 *===========================================================================*/
PUBLIC int do_exit(m_ptr)
message *m_ptr;			/* pointer to request message */
{
/* Handle sys_exit. A user process has exited or a system process requests 
 * to exit. Only the PM can request other process slots to be cleared.
 * The routine to clean up a process table slot cancels outstanding timers, 
 * possibly removes the process from the message queues, and resets certain 
 * process table fields to the default values.
 */
/*
 * 处理 sys_exit. 一个用户进程已经退出, 或者一个系统进程请求退出. 只有 PM 
 * 才有权力在进程表中清空进程槽. 该例程清扫进程表的进程槽并取消进程的定
 * 时器, 可能会从消息队列中移除进程, 将特定的进程表项复原到默认值.
 */
  int exit_proc_nr;				

  /* Determine what process exited. User processes are handled here. */
  /* 判断是哪个进程退出. 用户进程在这里处理. */
  // 如果向进程发出退出消息的源进程是 PM_PROC_NR, 则获取退出的进程号,
  // 如果退出的进程不是 PM, 则
  if (PM_PROC_NR == m_ptr->m_source) {
      exit_proc_nr = m_ptr->PR_PROC_NR;		/* get exiting process */
      if (exit_proc_nr != SELF) { 		/* PM tries to exit self */
      // 如果进程号无效, 则出错返回.
          if (! isokprocn(exit_proc_nr)) return(EINVAL);
	  // 清空与该进程有关的数据结构, 例如定时器, 特权级结构等.
          clear_proc(proc_addr(exit_proc_nr));	/* exit a user process */
          return(OK);				/* report back to PM */
      }
  } 

  /* The PM or some other system process requested to be exited. */
  clear_proc(proc_addr(m_ptr->m_source));
  return(EDONTREPLY);
}

/*===========================================================================*
 *			         clear_proc				     *
 *===========================================================================*/
PRIVATE void clear_proc(rc)
register struct proc *rc;		/* slot of process to clean up */
{
  register struct proc *rp;		/* iterate over process table */
  register struct proc **xpp;		/* iterate over caller queue */
  int i;

  /* Turn off any alarm timers at the clock. */   
  // 关闭进程的定时器
  reset_timer(&priv(rc)->s_alarm_timer);

  /* Make sure that the exiting process is no longer scheduled. */
  // 将正在退出的进程从调度队列中移除.
  if (rc->p_rts_flags == 0) lock_dequeue(rc);

  /* If the process being terminated happens to be queued trying to send a
   * message (e.g., the process was killed by a signal, rather than it doing 
   * a normal exit), then it must be removed from the message queues.
   */
  /*
   * 如果正在被终止的进程碰巧正在发送消息(如, 进程被一个信号杀死, 而不是
   * 主动正常的退出), 那么该进程就要从消息队列中移除.
   */
  if (rc->p_rts_flags & SENDING) {
      /* Check all proc slots to see if the exiting process is queued. */
      for (rp = BEG_PROC_ADDR; rp < END_PROC_ADDR; rp++) {
          if (rp->p_caller_q == NIL_PROC) continue;
          /* Make sure that the exiting process is not on the queue. */
          xpp = &rp->p_caller_q;
          while (*xpp != NIL_PROC) {		/* check entire queue */
              if (*xpp == rc) {			/* process is on the queue */
                  *xpp = (*xpp)->p_q_link;	/* replace by next process */
                  break;
              }
              xpp = &(*xpp)->p_q_link;		/* proceed to next queued */
          }
      }
  }

  /* Check the table with IRQ hooks to see if hooks should be released. */
  for (i=0; i < NR_IRQ_HOOKS; i++) {
      if (irq_hooks[i].proc_nr == proc_nr(rc)) {
          rm_irq_handler(&irq_hooks[i]);	/* remove interrupt handler */
          irq_hooks[i].proc_nr = NONE; 		/* mark hook as free */
      }
  }

  /* Now it is safe to release the process table slot. If this is a system 
   * process, also release its privilege structure.  Further cleanup is not
   * needed at this point. All important fields are reinitialized when the 
   * slots are assigned to another, new process. 
   */
  rc->p_rts_flags = SLOT_FREE;		
  if (priv(rc)->s_flags & SYS_PROC) priv(rc)->s_proc_nr = NONE;
}

#endif /* USE_EXIT */

