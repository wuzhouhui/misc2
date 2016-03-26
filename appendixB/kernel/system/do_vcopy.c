/* The kernel call implemented in this file:
 *   m_type:	SYS_VIRVCOPY, SYS_PHYSVCOPY 
 *
 * The parameters for this kernel call are:
 *    m1_i3:	VCP_VEC_SIZE		size of copy request vector 
 *    m1_p1:	VCP_VEC_ADDR		address of vector at caller 
 *    m1_i2:	VCP_NR_OK		number of successfull copies	
 */
/*
 * 该文件实现的系统调用:
 *	m_type:	SYS_VIRVCOPY, SYS_PHYSVCOPY
 *
 * 该系统调用的参数包括:
 *	m1_i3:	VCP_VEC_SIZE	复制请求向量的大小
 *	m1_p1:	VCP_VEC_ADDR	向量在主调进程中的地址
 *	m1_i2:	VCP_NR_OK	成功复制的数量
 */

#include "../system.h"
#include <minix/type.h>

#if (USE_VIRVCOPY || USE_PHYSVCOPY)

/* Buffer to hold copy request vector from user. */
/* 保存用户的复制请求的向量的缓冲区 */
PRIVATE struct vir_cp_req vir_cp_req[VCOPY_VEC_SIZE];

/*===========================================================================*
 *				do_vcopy				     *
 *===========================================================================*/
PUBLIC int do_vcopy(m_ptr)
register message *m_ptr;	/* pointer to request message */
{
/* Handle sys_virvcopy() and sys_physvcopy() that pass a vector with copy
 * requests. Although a single handler function is used, there are two
 * different kernel calls so that permissions can be checked.
 */
/*
 * 处理 sys_virvcopy() 与 sys_physvcopy(), 调用时需要传递带有复制请求
 * 的向量. 虽然使用了单一的处理函数, 但这是两个系统调用, 所以需要检查
 * 权限.
 */
  int nr_req;
  int caller_pid;
  vir_bytes caller_vir;
  phys_bytes caller_phys;
  phys_bytes kernel_phys;
  phys_bytes bytes;
  int i,s;
  struct vir_cp_req *req;

  /* Check if request vector size is ok. */
  nr_req = (unsigned) m_ptr->VCP_VEC_SIZE;
  if (nr_req > VCOPY_VEC_SIZE) return(EINVAL);
  bytes = nr_req * sizeof(struct vir_cp_req);

  /* Calculate physical addresses and copy (port,value)-pairs from user. */
  caller_pid = (int) m_ptr->m_source; 
  caller_vir = (vir_bytes) m_ptr->VCP_VEC_ADDR;
  caller_phys = umap_local(proc_addr(caller_pid), D, caller_vir, bytes);
  if (0 == caller_phys) return(EFAULT);
  kernel_phys = vir2phys(vir_cp_req);
  // 将用户的 "复制请求" 数组复制到内核的 "复制请求" 数组中.
  phys_copy(caller_phys, kernel_phys, (phys_bytes) bytes);

  /* Assume vector with requests is correct. Try to copy everything. */
  m_ptr->VCP_NR_OK = 0;
  for (i=0; i<nr_req; i++) {

      req = &vir_cp_req[i];

      /* Check if physical addressing is used without SYS_PHYSVCOPY. */
      if (((req->src.segment | req->dst.segment) & PHYS_SEG) &&
              m_ptr->m_type != SYS_PHYSVCOPY) return(EPERM);
      if ((s=virtual_copy(&req->src, &req->dst, req->count)) != OK) 
          return(s);
      m_ptr->VCP_NR_OK ++;
  }
  return(OK);
}

#endif /* (USE_VIRVCOPY || USE_PHYSVCOPY) */

