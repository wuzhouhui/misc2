/* The kernel call that is implemented in this file:
 *   m_type:	SYS_GETKSIG
 *
 * The parameters for this kernel call are:
 *     m2_i1:	SIG_PROC  	# process with pending signals
 *     m2_l1:	SIG_MAP		# bit map with pending signals
 */
/*
 * 该文件实现的系统调用:
 *	m_type:	SYS_GETKSIG
 *
 * 该系统调用的参数包括
 *	m2_i1:	SIG_PROC	拥有未决信号的进程数量
 *	m2_l1:	SIG_MAP		拥有未决信号的位图数量
 */

#include "../system.h"
#include <signal.h>
#include <sys/sigcontext.h>

#if USE_GETKSIG

/*===========================================================================*
 *			      do_getksig				     *
 *===========================================================================*/
PUBLIC int do_getksig(m_ptr)
message *m_ptr;			/* pointer to request message */
{
/* PM is ready to accept signals and repeatedly does a kernel call to get 
 * one. Find a process with pending signals. If no signals are available, 
 * return NONE in the process number field.
 * It is not sufficient to ready the process when PM is informed, because 
 * PM can block waiting for FS to do a core dump.
 */
/*
 * PM 准备好接收一个信号, 并且重复地调用系统调用, 每次调用获取一个信号.
 * 找到一个拥有未决信号的进程. 如果没有信号是可用的, 在进程号字段返回
 * NONE. 
 * 当 PM 收到通知时, 为进程就绪是不够的, 因为 PM 可以阻塞等待, 为 FS 
 * 作核心转储.
 */
  register struct proc *rp;

  /* Find the next process with pending signals. */
  for (rp = BEG_USER_ADDR; rp < END_PROC_ADDR; rp++) {
	// 进程有未决的信号
      if (rp->p_rts_flags & SIGNALED) {
          m_ptr->SIG_PROC = rp->p_nr;		/* store signaled process */
          m_ptr->SIG_MAP = rp->p_pending;	/* pending signals map */
          sigemptyset(&rp->p_pending); 		/* ball is in PM's court */
          rp->p_rts_flags &= ~SIGNALED;		/* blocked by SIG_PENDING */
          return(OK);
      }
  }

  /* No process with pending signals was found. */
  m_ptr->SIG_PROC = NONE; 
  return(OK);
}
#endif /* USE_GETKSIG */

