/* The kernel call implemented in this file:
 *   m_type:	SYS_SDEVIO
 *
 * The parameters for this kernel call are:
 *    m2_i3:	DIO_REQUEST	(request input or output)	
 *    m2_i1:	DIO_TYPE	(flag indicating byte, word, or long)
 *    m2_l1:	DIO_PORT	(port to read/ write)	
 *    m2_p1:	DIO_VEC_ADDR	(virtual address of buffer)	
 *    m2_l2:	DIO_VEC_SIZE	(number of elements)	
 *    m2_i2:	DIO_VEC_PROC	(process where buffer is)	
 */
/*
 * 该文件实现的系统调用:
 *	m_type:	SYS_SDEVIO
 *
 * 该系统调用的参数包括:
 *	m2_i3:	DIO_REQUEST	(请求输出或输入)
 *	m2_i1:	DIO_TYPE	(指示字节, 字 或 长字 的标志)
 *	m2_l1:	DIO_PORT	(读写怕端口号)
 *	m2_p1:	DIO_VEC_ADDR	(缓冲区的虚拟地址)
 *	m2_l2:	DIO_VEC_SIZE	(元素的数量)
 *	m2_i2:	DIO_VEC_PROC	(缓冲区的所属进程)
 */

#include "../system.h"
#include <minix/devio.h>

#if USE_SDEVIO

/*===========================================================================*
 *			        do_sdevio                                    *
 *===========================================================================*/
PUBLIC int do_sdevio(m_ptr)
register message *m_ptr;	/* pointer to request message */
{
  int proc_nr = m_ptr->DIO_VEC_PROC;
  int count = m_ptr->DIO_VEC_SIZE;
  long port = m_ptr->DIO_PORT;
  phys_bytes phys_buf;

  /* Check if process number is OK. A process number is allowed here, because
   * driver may directly provide a pointer to a buffer at the user-process
   * that initiated the device I/O. Kernel processes, of course, are denied.
   */
  /*
   * 检查进程是否合法. 这里允许一个进程号, 因为驱动程序可能直接提供一个
   * 缓冲区指针, 在一个初始化设备 I/O 的用户进程当中.
   */
  if (proc_nr == SELF) proc_nr = m_ptr->m_source;
  if (! isokprocn(proc_nr)) return(EINVAL);
  if (iskerneln(proc_nr)) return(EPERM);

  /* Get and check physical address. */
  if ((phys_buf = numap_local(proc_nr, (vir_bytes) m_ptr->DIO_VEC_ADDR, count)) == 0)
      return(EFAULT);

  /* Perform device I/O for bytes and words. Longs are not supported. */
  if (m_ptr->DIO_REQUEST == DIO_INPUT) { 
      switch (m_ptr->DIO_TYPE) {
		     // _phys_insb
      case DIO_BYTE: phys_insb(port, phys_buf, count); break; 
		     // _phys_insw
      case DIO_WORD: phys_insw(port, phys_buf, count); break; 
      default: return(EINVAL);
      } 
  } else if (m_ptr->DIO_REQUEST == DIO_OUTPUT) { 
      switch (m_ptr->DIO_TYPE) {
		     // _phys_outsb
      case DIO_BYTE: phys_outsb(port, phys_buf, count); break; 
		     // _phys_outsw
      case DIO_WORD: phys_outsw(port, phys_buf, count); break; 
      default: return(EINVAL);
      } 
  }
  else {
      return(EINVAL);
  }
  return(OK);
}

#endif /* USE_SDEVIO */

