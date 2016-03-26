/* The kernel call that is implemented in this file:
 *   m_type:	SYS_KILL
 *
 * The parameters for this kernel call are:
 *     m2_i1:	SIG_PROC  	# process to signal/ pending		
 *     m2_i2:	SIG_NUMBER	# signal number to send to process
 */
/*
 * 该文件实现的系统调用:
 *	m_type:	SYS_KILL
 *
 * 该系统调用的参数包括:
 *	m2_i1:	SIG_PROC	接收信号或未决消息的进程号
 *	m2_i2:	SIG_NUMBER	发送给进程的信号值
 */

#include "../system.h"
#include <signal.h>
#include <sys/sigcontext.h>

#if USE_KILL

/*===========================================================================*
 *			          do_kill				     *
 *===========================================================================*/
PUBLIC int do_kill(m_ptr)
message *m_ptr;			/* pointer to request message */
{
/* Handle sys_kill(). Cause a signal to be sent to a process. The PM is the
 * central server where all signals are processed and handler policies can
 * be registered. Any request, except for PM requests, is added to the map
 * of pending signals and the PM is informed about the new signal.
 * Since system servers cannot use normal POSIX signal handlers (because they
 * are usually blocked on a RECEIVE), they can request the PM to transform 
 * signals into messages. This is done by the PM with a call to sys_kill(). 
 */
 /*
  * 处理 sys_kill(). 该系统调用导致一个信号被发送给进程. PM 是处理所有信号
  * 与注册处理程序策略的中心服务程序. 所有的请求, 除了 PM 的请求, 都会被
  * 添加至未决信号位图中, 当新的信号到来时, PM 就会收到通知. 因为系统服务
  * 进程不能使用通常的 POSIX 信号处理程序(因为它们经常阻塞在 RECEIVE 上),
  * 它们可以请求 PM 将信号变换到消息中. 这是 PM 通过调用 sys_kill() 完成
  * 的.
  */
  proc_nr_t proc_nr = m_ptr->SIG_PROC;
  int sig_nr = m_ptr->SIG_NUMBER;

  if (! isokprocn(proc_nr) || sig_nr > _NSIG) return(EINVAL);
  if (iskerneln(proc_nr)) return(EPERM);

  if (m_ptr->m_source == PM_PROC_NR) {
      /* Directly send signal notification to a system process. */
      // PM 只能向 SYS_PROC 发送信号.
      if (! (priv(proc_addr(proc_nr))->s_flags & SYS_PROC)) return(EPERM);
      send_sig(proc_nr, sig_nr);
  } else {
      /* Set pending signal to be processed by the PM. */
      cause_sig(proc_nr, sig_nr);
  }
  return(OK);
}

#endif /* USE_KILL */

