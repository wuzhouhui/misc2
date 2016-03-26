/* The kernel call implemented in this file:
 *   m_type:	SYS_VDEVIO
 *
 * The parameters for this kernel call are:
 *    m2_i3:	DIO_REQUEST	(request input or output)	
 *    m2_i1:	DIO_TYPE	(flag indicating byte, word, or long)
 *    m2_p1:	DIO_VEC_ADDR	(pointer to port/ value pairs)	
 *    m2_i2:	DIO_VEC_SIZE	(number of ports to read or write) 
 */
/*
 * 该文件实现的系统调用:
 *	m_type:	SYS_VDEVIO
 *
 * 该系统调用的参数包括:
 *	m2_i3:	DIO_REQUEST	(请求输入或输出)
 *	m2_i1:	DIO_TYPE	(标志, 指示 字节, 字 或 长字)
 *	m2_p1:	DIO_VEC_ADDR	(指针, 指向 端口-值 对)
 *	m2_i2:	DIO_VEC_SIZE	(读写的端口数)
 */

#include "../system.h"
#include <minix/devio.h>

#if USE_VDEVIO

/* Buffer for SYS_VDEVIO to copy (port,value)-pairs from/ to user. */
PRIVATE char vdevio_buf[VDEVIO_BUF_SIZE];      
PRIVATE pvb_pair_t *pvb = (pvb_pair_t *) vdevio_buf;           
PRIVATE pvw_pair_t *pvw = (pvw_pair_t *) vdevio_buf;      
PRIVATE pvl_pair_t *pvl = (pvl_pair_t *) vdevio_buf;     

/*===========================================================================*
 *			        do_vdevio                                    *
 *===========================================================================*/
PUBLIC int do_vdevio(m_ptr)
register message *m_ptr;	/* pointer to request message */
{
/* Perform a series of device I/O on behalf of a non-kernel process. The 
 * I/O addresses and I/O values are fetched from and returned to some buffer
 * in user space. The actual I/O is wrapped by lock() and unlock() to prevent
 * that I/O batch from being interrrupted.
 * This is the counterpart of do_devio, which performs a single device I/O. 
 */ 
/*
 * 代表非内核进程执行一系列设备 I/O 操作. I/O 地址与 I/O 值从用户空间的
 * 缓冲区中获取, 也可以返回给用户空间的缓冲区. 真正的 I/O 被 lock() 和
 * unlock() 包裹, 避免 I/O 批处理被中断.
 */
  int vec_size;               /* size of vector */
  int io_in;                  /* true if input */
  size_t bytes;               /* # bytes to be copied */
  int caller_proc;            /* process number of caller */
  vir_bytes caller_vir;       /* virtual address at caller */
  phys_bytes caller_phys;     /* physical address at caller */
  int i;
    
  /* Get the request, size of the request vector, and check the values. */
  if (m_ptr->DIO_REQUEST == DIO_INPUT) io_in = TRUE;
  else if (m_ptr->DIO_REQUEST == DIO_OUTPUT) io_in = FALSE;
  else return(EINVAL);
  if ((vec_size = m_ptr->DIO_VEC_SIZE) <= 0) return(EINVAL);
  switch (m_ptr->DIO_TYPE) {
      case DIO_BYTE: bytes = vec_size * sizeof(pvb_pair_t); break;
      case DIO_WORD: bytes = vec_size * sizeof(pvw_pair_t); break;
      case DIO_LONG: bytes = vec_size * sizeof(pvl_pair_t); break;
      default:  return(EINVAL);   /* check type once and for all */
  }
  if (bytes > sizeof(vdevio_buf))  return(E2BIG);

  /* Calculate physical addresses and copy (port,value)-pairs from user. */
  caller_proc = m_ptr->m_source; 
  caller_vir = (vir_bytes) m_ptr->DIO_VEC_ADDR;
  caller_phys = umap_local(proc_addr(caller_proc), D, caller_vir, bytes);
  if (0 == caller_phys) return(EFAULT);
  // _phys_copy, 从 进程空间复制到 缓冲区.
  phys_copy(caller_phys, vir2phys(vdevio_buf), (phys_bytes) bytes);

  /* Perform actual device I/O for byte, word, and long values. Note that 
   * the entire switch is wrapped in lock() and unlock() to prevent the I/O
   * batch from being interrupted. 
   */  
  lock(13, "do_vdevio");
  switch (m_ptr->DIO_TYPE) {
  case DIO_BYTE: 					 /* byte values */
      if (io_in) for (i=0; i<vec_size; i++)  pvb[i].value = inb(pvb[i].port); 
      else       for (i=0; i<vec_size; i++)  outb(pvb[i].port, pvb[i].value); 
      break; 
  case DIO_WORD:					  /* word values */
      if (io_in) for (i=0; i<vec_size; i++)  pvw[i].value = inw(pvw[i].port);  
      else       for (i=0; i<vec_size; i++)  outw(pvw[i].port, pvw[i].value); 
      break; 
  default:            					  /* long values */
      if (io_in) for (i=0; i<vec_size; i++) pvl[i].value = inl(pvl[i].port);  
      else       for (i=0; i<vec_size; i++) outl(pvb[i].port, pvl[i].value); 
  }
  unlock(13);
    
  /* Almost done, copy back results for input requests. */
  if (io_in) phys_copy(vir2phys(vdevio_buf), caller_phys, (phys_bytes) bytes);
  return(OK);
}

#endif /* USE_VDEVIO */

