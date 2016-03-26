/* The kernel call implemented in this file:
 *   m_type:	SYS_SETALARM 
 *
 * The parameters for this kernel call are:
 *    m2_l1:	ALRM_EXP_TIME		(alarm's expiration time)
 *    m2_i2:	ALRM_ABS_TIME		(expiration time is absolute?)
 *    m2_l1:	ALRM_TIME_LEFT		(return seconds left of previous)
 */
/*
 * 该文件实现的系统调用:
 *	m_type:	SYS_SETALARM 
 *
 * 该系统调用的参数:
 *	m2_l1:	ALRM_EXP_TIME		(闹钟的到期时间)
 *	m2_i2:	ALRM_ABS_TIME		(到期时间是否是绝对时间)
 *	m2_i2:	ALRM_TIME_LEFT		(返回上一个闹钟的剩余时间)
 */

#include "../system.h"

#if USE_SETALARM

FORWARD _PROTOTYPE( void cause_alarm, (timer_t *tp) );

/*===========================================================================*
 *				do_setalarm				     *
 *===========================================================================*/
PUBLIC int do_setalarm(m_ptr)
message *m_ptr;			/* pointer to request message */
{
/* A process requests a synchronous alarm, or wants to cancel its alarm. */
/* 一个进程请求一个同步闹钟, 或者想要取消它自己的闹钟. */
  register struct proc *rp;	/* pointer to requesting process */
  int proc_nr;			/* which process wants the alarm */
  long exp_time;		/* expiration time for this alarm */
  int use_abs_time;		/* use absolute or relative time */
  timer_t *tp;			/* the process' timer structure */
  clock_t uptime;		/* placeholder for current uptime */

  /* Extract shared parameters from the request message. */
  exp_time = m_ptr->ALRM_EXP_TIME;	/* alarm's expiration time */
  use_abs_time = m_ptr->ALRM_ABS_TIME;	/* flag for absolute time */
  proc_nr = m_ptr->m_source;		/* process to interrupt later */
  rp = proc_addr(proc_nr);
  if (! (priv(rp)->s_flags & SYS_PROC)) return(EPERM);

  /* Get the timer structure and set the parameters for this alarm. */
  tp = &(priv(rp)->s_alarm_timer);	
  tmr_arg(tp)->ta_int = proc_nr;	
  tp->tmr_func = cause_alarm; 

  /* Return the ticks left on the previous alarm. */
  uptime = get_uptime(); 
  if ((tp->tmr_exp_time != TMR_NEVER) && (uptime < tp->tmr_exp_time) ) {
      m_ptr->ALRM_TIME_LEFT = (tp->tmr_exp_time - uptime);
  } else {
      m_ptr->ALRM_TIME_LEFT = 0;
  }

  /* Finally, (re)set the timer depending on the expiration time. */
  if (exp_time == 0) {
      reset_timer(tp);
  } else {
      tp->tmr_exp_time = (use_abs_time) ? exp_time : exp_time + get_uptime();
      set_timer(tp, tp->tmr_exp_time, tp->tmr_func);
  }
  return(OK);
}

/*===========================================================================*
 *				cause_alarm				     *
 *===========================================================================*/
PRIVATE void cause_alarm(tp)
timer_t *tp;
{
/* Routine called if a timer goes off and the process requested a synchronous
 * alarm. The process number is stored in timer argument 'ta_int'. Notify that
 * process with a notification message from CLOCK.
 */
/*
 * 当某个定时器到期, 并且进程请求一个同步闹钟时, 调用该例程. 进程号存储
 * 在定时器的参数 'ta_int' 中. 向进程发送一个通知, 这个通知由 CLOCK 
 * 发出.
 */
  int proc_nr = tmr_arg(tp)->ta_int;		/* get process number */
  lock_notify(CLOCK, proc_nr);			/* notify process */
}

#endif /* USE_SETALARM */
