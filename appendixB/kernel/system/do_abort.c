/* The kernel call implemented in this file:
 *   m_type:	SYS_ABORT
 *
 * The parameters for this kernel call are:
 *    m1_i1:	ABRT_HOW 	(how to abort, possibly fetch monitor params)	
 *    m1_i2:	ABRT_MON_PROC 	(proc nr to get monitor params from)	
 *    m1_i3:	ABRT_MON_LEN	(length of monitor params)
 *    m1_p1:	ABRT_MON_ADDR 	(virtual address of params)	
 */
/*
 * 在这个文件中实现的系统调用是:
 *	m_type:	SYS_ABORT
 * 
 * 该系统调用的参数包括:
 *	m1_i1:	ABRT_HOW	(如何中止, 可能是抓取监视器参数)
 *	m1_i2:	ABRT_MON_PROC	(从哪个进程获取监视器参数)
 *	m1_i3:	ABRT_MON_LEN	(监视器参数的长度)
 *	m1_p1:	ABRT_MON_ADDR	(参数的虚拟地址)
 */

#include "../system.h"
#include <unistd.h>

#if USE_ABORT

/*===========================================================================*
 *				do_abort				     *
 *===========================================================================*/
PUBLIC int do_abort(m_ptr)
message *m_ptr;			/* pointer to request message */
{
  /* Handle sys_abort. MINIX is unable to continue. This can originate in the
   * PM (normal abort or panic) or TTY (after CTRL-ALT-DEL). 
   */
  /*
   * 处理 sys_abort. MINIX 无法继续运行下去. 这个系统调用可以在 PM 或 TTY
   * 中产生.
   */
  int how = m_ptr->ABRT_HOW;
  int proc_nr;
  int length;
  phys_bytes src_phys;

  /* See if the monitor is to run the specified instructions. */
  /* 检查监视器是否需要运行特定的指针 */
  if (how == RBT_MONITOR) {

      proc_nr = m_ptr->ABRT_MON_PROC;
      // 检查进程号的有效性
      if (! isokprocn(proc_nr)) return(EINVAL);
      length = m_ptr->ABRT_MON_LEN + 1;
      if (length > kinfo.params_size) return(E2BIG);
      src_phys = numap_local(proc_nr,(vir_bytes)m_ptr->ABRT_MON_ADDR,length);
      if (! src_phys) return(EFAULT);

      /* Parameters seem ok, copy them and prepare shutting down. */
      // 从 proc_nr 处获取参数并存放于内核信息结构中
      phys_copy(src_phys, kinfo.params_base, (phys_bytes) length);
  }

  /* Now prepare to shutdown MINIX. */
  prepare_shutdown(how);
  return(OK);				/* pro-forma (really EDISASTER) */
}

#endif /* USE_ABORT */

