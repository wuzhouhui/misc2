/* The kernel call implemented in this file:
 *   m_type:	SYS_VIRCOPY, SYS_PHYSCOPY
 *
 * The parameters for this kernel call are:
 *    m5_c1:	CP_SRC_SPACE		source virtual segment
 *    m5_l1:	CP_SRC_ADDR		source offset within segment
 *    m5_i1:	CP_SRC_PROC_NR		source process number
 *    m5_c2:	CP_DST_SPACE		destination virtual segment
 *    m5_l2:	CP_DST_ADDR		destination offset within segment
 *    m5_i2:	CP_DST_PROC_NR		destination process number
 *    m5_l3:	CP_NR_BYTES		number of bytes to copy
 */
/*
 * 该文件实现的系统调用:
 *	m_type:	SYS_VIRCOPY, SYS_PHYSCOPY
 *
 * 该系统调用的参数包括:
 *	m5_c1:	CP_SRC_SPACE		源虚拟段地址
 *	m5_l1:	CP_SRC_ADDR		源段内偏移地址
 *	m5_i1:	CP_SRC_PROC_NR		源进程号
 *	m5_c2:	CP_DST_SPACE		目标虚拟段地址
 *	m5_l2:	CP_DST_ADDR		目标段内偏移地址
 *	m5_i2:	CP_DST_PROC_NR		目标进程号
 *	m5_l3:	CP_NR_BYTES		需要复制的字节数
 */

#include "../system.h"
#include <minix/type.h>

#if (USE_VIRCOPY || USE_PHYSCOPY)

/*===========================================================================*
 *				do_copy					     *
 *===========================================================================*/
PUBLIC int do_copy(m_ptr)
register message *m_ptr;	/* pointer to request message */
{
/* Handle sys_vircopy() and sys_physcopy().  Copy data using virtual or
 * physical addressing. Although a single handler function is used, there 
 * are two different kernel calls so that permissions can be checked. 
 */
/*
 * 处理 sys_vircopy() 与 sys_physcopy(). 使用虚拟或物理地址复制数据. 虽然
 * 只用一个处理函数, 但是实现了两个不同的系统调用, 于是需要检查权限.
 */
  struct vir_addr vir_addr[2];	/* virtual source and destination address */
  phys_bytes bytes;		/* number of bytes to copy */
  int i;

  /* Dismember the command message. */
  // 分解消息中包含的信息
  vir_addr[_SRC_].proc_nr = m_ptr->CP_SRC_PROC_NR;
  vir_addr[_SRC_].segment = m_ptr->CP_SRC_SPACE;
  vir_addr[_SRC_].offset = (vir_bytes) m_ptr->CP_SRC_ADDR;
  vir_addr[_DST_].proc_nr = m_ptr->CP_DST_PROC_NR;
  vir_addr[_DST_].segment = m_ptr->CP_DST_SPACE;
  vir_addr[_DST_].offset = (vir_bytes) m_ptr->CP_DST_ADDR;
  bytes = (phys_bytes) m_ptr->CP_NR_BYTES;

  /* Now do some checks for both the source and destination virtual address.
   * This is done once for _SRC_, then once for _DST_. 
   */
  /*
   * 现在为源与目标虚拟地址作一些检查. 先检查 _SRC_, 再检查 _DST_.
   */
  for (i=_SRC_; i<=_DST_; i++) {

      /* Check if process number was given implictly with SELF and is valid. */
      if (vir_addr[i].proc_nr == SELF) vir_addr[i].proc_nr = m_ptr->m_source;
      // 如果进程号无效, 并且段不是 PHYS_SEG, 则出错返回.
      if (! isokprocn(vir_addr[i].proc_nr) && vir_addr[i].segment != PHYS_SEG) 
          return(EINVAL); 

      /* Check if physical addressing is used without SYS_PHYSCOPY. */
      /* 检查物理地址是否可用 SYS_PHYSCOPY */
      if ((vir_addr[i].segment & PHYS_SEG) &&
          m_ptr->m_type != SYS_PHYSCOPY) return(EPERM);
  }

  /* Check for overflow. This would happen for 64K segments and 16-bit 
   * vir_bytes. Especially copying by the PM on do_fork() is affected. 
   */
  /*
   * 检查溢出. 对 64K 的段或 16 位的虚拟地址有可能发生溢出. 尤其会影响
   * 在调用 do_fork() 时 PM 的复制.
   */
  if (bytes != (vir_bytes) bytes) return(E2BIG);

  /* Now try to make the actual virtual copy. */
  /* 现在开始真正的虚拟地址复制. */
  return( virtual_copy(&vir_addr[_SRC_], &vir_addr[_DST_], bytes) );
}
#endif /* (USE_VIRCOPY || USE_PHYSCOPY) */

