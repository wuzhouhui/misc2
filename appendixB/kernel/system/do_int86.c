/* The kernel call implemented in this file:
 *   m_type:	SYS_INT86
 *
 * The parameters for this kernel call are:
 *    m1_p1:	INT86_REG86     
 */
/*
 * 该文件实现的系统调用:
 *	m_type:	SYS_INT86
 *	
 * 该系统调用的参数包括:
 *	m1_p1:	INT86_REG86
 */

#include "../system.h"
#include <minix/type.h>
#include <ibm/int86.h>

struct reg86u reg86;

/*===========================================================================*
 *				do_int86					     *
 *===========================================================================*/
PUBLIC int do_int86(m_ptr)
register message *m_ptr;	/* pointer to request message */
{
  int caller;
  vir_bytes caller_vir;
  phys_bytes caller_phys, kernel_phys;

  caller = (int) m_ptr->m_source; 
  caller_vir = (vir_bytes) m_ptr->INT86_REG86;
  caller_phys = umap_local(proc_addr(caller), D, caller_vir, sizeof(reg86));
  if (0 == caller_phys) return(EFAULT);
  kernel_phys = vir2phys(&reg86);
  phys_copy(caller_phys, kernel_phys, (phys_bytes) sizeof(reg86));

  // _level0 _int86 ???
  level0(int86);

  /* Copy results back to the caller */
  phys_copy(kernel_phys, caller_phys, (phys_bytes) sizeof(reg86));

  /* The BIOS call eats interrupts. Call get_randomness to generate some
   * entropy. Normally, get_randomness is called from an interrupt handler.
   * Figuring out the exact source is too complicated. CLOCK_IRQ is normally
   * not very random.
   */
  /*
   * BIOS 调用会吃掉中断. 调用 get_randomness() 来产生一些熵. 一般来说,
   * get_randomness() 是从一个中断处理函数中调用. 算出准备的源实在是太复
   * 杂了. CLOCK_IRQ 通常并不是非常随机.
   */
  lock(0, "do_int86");
  get_randomness(CLOCK_IRQ);
  unlock(0);

  return(OK);
}
