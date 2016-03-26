/* The kernel call implemented in this file:
 *   m_type:	SYS_TRACE
 *
 * The parameters for this kernel call are:
 *    m2_i1:	CTL_PROC_NR	process that is traced
 *    m2_i2:    CTL_REQUEST	trace request
 *    m2_l1:    CTL_ADDRESS     address at traced process' space
 *    m2_l2:    CTL_DATA        data to be written or returned here
 */
/*
 * 该文件实现的系统调用:
 *	m_type:	SYS_TRACE
 *
 * 该系统调用的参数包括:
 *	m2_i1:	CTL_PROC_NR	被跟踪的进程
 *	m2_i2:	CTL_REQUEST	跟踪请求
 *	m2_l1:	CTL_ADDRESS	被跟踪的进程地址空间内的某个地址
 *	m2_l2:	CTL_DATA	将被写的数据, 或返回值
 */

#include "../system.h"
#include <sys/ptrace.h>

#if USE_TRACE

/*==========================================================================*
 *				do_trace				    *
 *==========================================================================*/
#define TR_VLSIZE	((vir_bytes) sizeof(long))

PUBLIC int do_trace(m_ptr)
register message *m_ptr;
{
/* Handle the debugging commands supported by the ptrace system call
 * The commands are:
 * T_STOP	stop the process
 * T_OK		enable tracing by parent for this process
 * T_GETINS	return value from instruction space
 * T_GETDATA	return value from data space
 * T_GETUSER	return value from user process table
 * T_SETINS	set value from instruction space
 * T_SETDATA	set value from data space
 * T_SETUSER	set value in user process table
 * T_RESUME	resume execution
 * T_EXIT	exit
 * T_STEP	set trace bit
 *
 * The T_OK and T_EXIT commands are handled completely by the process manager,
 * all others come here.
 */
/*
 * 通过 ptrace 系统调用, 可以支持调试指令. 调用指令包括:
 *	T_STOP		停止进程
 *	T_OK		允许父进程跟子进程
 *	T_GETINS	从指令空间返回某个值
 *	T_GETDATA	从数据空间返回某个值
 *	T_GETUSER	从用户进程表中返回某个值
 *	T_SETINS	从指令空间设置某个值
 *	T_SETDATA	从数据空间设置某个值
 *	T_RESUME	重新开始运行
 *	T_EXIT		退出
 *	T_STEP		设置跟踪位
 *
 * T_OK 与 T_EXIT 命令完全由 PM 处理, 其余的都在这儿(???).
 */

  register struct proc *rp;
  phys_bytes src, dst;
  vir_bytes tr_addr = (vir_bytes) m_ptr->CTL_ADDRESS;
  long tr_data = m_ptr->CTL_DATA;
  int tr_request = m_ptr->CTL_REQUEST;
  int tr_proc_nr = m_ptr->CTL_PROC_NR;
  int i;

  if (! isokprocn(tr_proc_nr)) return(EINVAL);
  if (iskerneln(tr_proc_nr)) return(EPERM);

  rp = proc_addr(tr_proc_nr);
  if (isemptyp(rp)) return(EIO);
  switch (tr_request) {
  case T_STOP:			/* stop process */
	if (rp->p_rts_flags == 0) lock_dequeue(rp);
	rp->p_rts_flags |= P_STOP;
	rp->p_reg.psw &= ~TRACEBIT;	/* clear trace bit */
	return(OK);

  case T_GETINS:		/* return value from instruction space */
	if (rp->p_memmap[T].mem_len != 0) {
		if ((src = umap_local(rp, T, tr_addr, TR_VLSIZE)) == 0) return(EIO);
		phys_copy(src, vir2phys(&tr_data), (phys_bytes) sizeof(long));
		m_ptr->CTL_DATA = tr_data;
		break;
	}
	/* Text space is actually data space - fall through. */

  case T_GETDATA:		/* return value from data space */
	if ((src = umap_local(rp, D, tr_addr, TR_VLSIZE)) == 0) return(EIO);
	phys_copy(src, vir2phys(&tr_data), (phys_bytes) sizeof(long));
	m_ptr->CTL_DATA= tr_data;
	break;

  case T_GETUSER:		/* return value from process table */
	if ((tr_addr & (sizeof(long) - 1)) != 0 ||
	    tr_addr > sizeof(struct proc) - sizeof(long))
		return(EIO);
	m_ptr->CTL_DATA = *(long *) ((char *) rp + (int) tr_addr);
	break;

  case T_SETINS:		/* set value in instruction space */
	if (rp->p_memmap[T].mem_len != 0) {
		if ((dst = umap_local(rp, T, tr_addr, TR_VLSIZE)) == 0) return(EIO);
		phys_copy(vir2phys(&tr_data), dst, (phys_bytes) sizeof(long));
		m_ptr->CTL_DATA = 0;
		break;
	}
	/* Text space is actually data space - fall through. */

  case T_SETDATA:			/* set value in data space */
	if ((dst = umap_local(rp, D, tr_addr, TR_VLSIZE)) == 0) return(EIO);
	phys_copy(vir2phys(&tr_data), dst, (phys_bytes) sizeof(long));
	m_ptr->CTL_DATA = 0;
	break;

  case T_SETUSER:			/* set value in process table */
	if ((tr_addr & (sizeof(reg_t) - 1)) != 0 ||
	     tr_addr > sizeof(struct stackframe_s) - sizeof(reg_t))
		return(EIO);
	i = (int) tr_addr;
#if (CHIP == INTEL)
	/* Altering segment registers might crash the kernel when it
	 * tries to load them prior to restarting a process, so do
	 * not allow it.
	 */
	if (i == (int) &((struct proc *) 0)->p_reg.cs ||
	    i == (int) &((struct proc *) 0)->p_reg.ds ||
	    i == (int) &((struct proc *) 0)->p_reg.es ||
#if _WORD_SIZE == 4
	    i == (int) &((struct proc *) 0)->p_reg.gs ||
	    i == (int) &((struct proc *) 0)->p_reg.fs ||
#endif
	    i == (int) &((struct proc *) 0)->p_reg.ss)
		return(EIO);
#endif
	if (i == (int) &((struct proc *) 0)->p_reg.psw)
		/* only selected bits are changeable */
		SETPSW(rp, tr_data);
	else
		*(reg_t *) ((char *) &rp->p_reg + i) = (reg_t) tr_data;
	m_ptr->CTL_DATA = 0;
	break;

  case T_RESUME:		/* resume execution */
	rp->p_rts_flags &= ~P_STOP;
	if (rp->p_rts_flags == 0) lock_enqueue(rp);
	m_ptr->CTL_DATA = 0;
	break;

  case T_STEP:			/* set trace bit */
	rp->p_reg.psw |= TRACEBIT;
	rp->p_rts_flags &= ~P_STOP;
	if (rp->p_rts_flags == 0) lock_enqueue(rp);
	m_ptr->CTL_DATA = 0;
	break;

  default:
	return(EIO);
  }
  return(OK);
}

#endif /* USE_TRACE */
