/* The kernel call implemented in this file:
 *   m_type:	SYS_UMAP
 *
 * The parameters for this kernel call are:
 *    m5_i1:	CP_SRC_PROC_NR	(process number)	
 *    m5_c1:	CP_SRC_SPACE	(segment where address is: T, D, or S)
 *    m5_l1:	CP_SRC_ADDR	(virtual address)	
 *    m5_l2:	CP_DST_ADDR	(returns physical address)	
 *    m5_l3:	CP_NR_BYTES	(size of datastructure) 	
 */
/*
 * 该文件实现的系统调用包括:
 *	m_type:	SYS_UMAP
 *
 * 该系统调用的参数包括:
 *	m5_i1:	CP_SRC_PROC_NR	(进程号)
 *	m5_c1:	CP_SRC_SPACE	(地址所在的段: T, D, 或 S)
 *	m5_l1:	CP_SRC_ADDR	(虚拟地址)
 *	m5_l2:	CP_DST_ADDR	(返回物理地址)
 *	m5_l3:	CP_NR_BYTES	(数据结构的大小)
 */

#include "../system.h"

#if USE_UMAP

/*==========================================================================*
 *				do_umap					    *
 *==========================================================================*/
PUBLIC int do_umap(m_ptr)
register message *m_ptr;	/* pointer to request message */
{
/* Map virtual address to physical, for non-kernel processes. */
/* 将虚拟地址映射到物理地址, 由非内核进程使用. */
  int seg_type = m_ptr->CP_SRC_SPACE & SEGMENT_TYPE;
  int seg_index = m_ptr->CP_SRC_SPACE & SEGMENT_INDEX;
  vir_bytes offset = m_ptr->CP_SRC_ADDR;
  int count = m_ptr->CP_NR_BYTES;
  int proc_nr = (int) m_ptr->CP_SRC_PROC_NR;
  phys_bytes phys_addr;

  /* Verify process number. */
  if (proc_nr == SELF) proc_nr = m_ptr->m_source;
  if (! isokprocn(proc_nr)) return(EINVAL);

  /* See which mapping should be made. */
  switch(seg_type) {
  case LOCAL_SEG:
      phys_addr = umap_local(proc_addr(proc_nr), seg_index, offset, count); 
      break;
  case REMOTE_SEG:
      phys_addr = umap_remote(proc_addr(proc_nr), seg_index, offset, count); 
      break;
  case BIOS_SEG:
      phys_addr = umap_bios(proc_addr(proc_nr), offset, count); 
      break;
  default:
      return(EINVAL);
  }
  m_ptr->CP_DST_ADDR = phys_addr;
  return (phys_addr == 0) ? EFAULT: OK;
}

#endif /* USE_UMAP */
