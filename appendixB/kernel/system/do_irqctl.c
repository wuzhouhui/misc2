/* The kernel call implemented in this file:
 *   m_type:	SYS_IRQCTL
 *
 * The parameters for this kernel call are:
 *    m5_c1:	IRQ_REQUEST	(control operation to perform)	
 *    m5_c2:	IRQ_VECTOR	(irq line that must be controlled)
 *    m5_i1:	IRQ_POLICY	(irq policy allows reenabling interrupts)
 *    m5_l3:	IRQ_HOOK_ID	(provides index to be returned on interrupt)
 *      ,,          ,,          (returns index of irq hook assigned at kernel)
 */
/*
 * 该文件实现的系统调用是:
 *	m_type:	SYS_IRQCTL
 *
 * 该系统调用的参数包括:
 *	m5_c1:	IRQ_REQUEST	(所要执行的控制操作)
 *	m5_c2:	IRQ_VECTOR	(必须控制的 irq 线路)
 *	m5_i1:	IRQ_POLICY	(irq 策略可以重新开启中断)
 *	m5_l3:	IRQ_HOOK_ID	(在中断时, 提供返回的索引)
 *	  ,,	    ,,		(返回 irq 钩子的索引)
 */

#include "../system.h"

#if USE_IRQCTL

FORWARD _PROTOTYPE(int generic_handler, (irq_hook_t *hook));

/*===========================================================================*
 *				do_irqctl				     *
 *===========================================================================*/
PUBLIC int do_irqctl(m_ptr)
register message *m_ptr;	/* pointer to request message */
{
  /* Dismember the request message. */
  /* 分解请求消息. */
  int irq_vec;
  int irq_hook_id;
  int notify_id;
  int r = OK;
  irq_hook_t *hook_ptr;

  /* Hook identifiers start at 1 and end at NR_IRQ_HOOKS. */
  /* 钩子的标识符从 1 开始, 终止于 NR_IRQ_HOOKS */
  irq_hook_id = (unsigned) m_ptr->IRQ_HOOK_ID - 1;
  irq_vec = (unsigned) m_ptr->IRQ_VECTOR; 

  /* See what is requested and take needed actions. */
  switch(m_ptr->IRQ_REQUEST) {

  /* Enable or disable IRQs. This is straightforward. */
  case IRQ_ENABLE:           
  case IRQ_DISABLE: 
      if (irq_hook_id >= NR_IRQ_HOOKS ||
          irq_hooks[irq_hook_id].proc_nr == NONE) return(EINVAL);
      if (irq_hooks[irq_hook_id].proc_nr != m_ptr->m_source) return(EPERM);
      if (m_ptr->IRQ_REQUEST == IRQ_ENABLE)
	// _enable_irq
          enable_irq(&irq_hooks[irq_hook_id]);	
      else 
	// _disable_irq
          disable_irq(&irq_hooks[irq_hook_id]);	
      break;

  /* Control IRQ policies. Set a policy and needed details in the IRQ table.
   * This policy is used by a generic function to handle hardware interrupts. 
   */
  /*
   * 控制 IRQ 策略. 在 IRQ 表中设置一个策略, 并且需要一些详细信息. 这个策略
   * 由通用的函数使用, 用来处理硬件中断.
   */
  case IRQ_SETPOLICY:  

      /* Check if IRQ line is acceptable. */
      if (irq_vec < 0 || irq_vec >= NR_IRQ_VECTORS) return(EINVAL);

      /* Find a free IRQ hook for this mapping. */
      hook_ptr = NULL;
      for (irq_hook_id=0; irq_hook_id<NR_IRQ_HOOKS; irq_hook_id++) {
          if (irq_hooks[irq_hook_id].proc_nr == NONE) {	
              hook_ptr = &irq_hooks[irq_hook_id];	/* free hook */
              break;
          }
      }
      if (hook_ptr == NULL) return(ENOSPC);

      /* When setting a policy, the caller must provide an identifier that
       * is returned on the notification message if a interrupt occurs.
       */
      /*
       * 当设置一个策略时, 主调进程必须提供一个标识符, 当一个中断发生时,
       * 这个标识符在通知消息中返回.
       */
      notify_id = (unsigned) m_ptr->IRQ_HOOK_ID;
      if (notify_id > CHAR_BIT * sizeof(irq_id_t) - 1) return(EINVAL);

      /* Install the handler. */
      hook_ptr->proc_nr = m_ptr->m_source;	/* process to notify */   	
      hook_ptr->notify_id = notify_id;		/* identifier to pass */   	
      hook_ptr->policy = m_ptr->IRQ_POLICY;	/* policy for interrupts */
      put_irq_handler(hook_ptr, irq_vec, generic_handler);

      /* Return index of the IRQ hook in use. */
      m_ptr->IRQ_HOOK_ID = irq_hook_id + 1;
      break;

  case IRQ_RMPOLICY:
      if (irq_hook_id >= NR_IRQ_HOOKS ||
               irq_hooks[irq_hook_id].proc_nr == NONE) {
           return(EINVAL);
      } else if (m_ptr->m_source != irq_hooks[irq_hook_id].proc_nr) {
           return(EPERM);
      }
      /* Remove the handler and return. */
      rm_irq_handler(&irq_hooks[irq_hook_id]);
      break;

  default:
      r = EINVAL;				/* invalid IRQ_REQUEST */
  }
  return(r);
}

/*===========================================================================*
 *			       generic_handler				     *
 *===========================================================================*/
PRIVATE int generic_handler(hook)
irq_hook_t *hook;	
{
/* This function handles hardware interrupt in a simple and generic way. All
 * interrupts are transformed into messages to a driver. The IRQ line will be
 * reenabled if the policy says so.
 */
/*
 * 这个函数用一个简单通用的方式处理硬件中断. 所有的中断都被装配进消息中,
 * 传递给驱动程序. 如果策略允许, IRQ 引脚会重新使能.
 */

  /* As a side-effect, the interrupt handler gathers random information by 
   * timestamping the interrupt events. This is used for /dev/random.
   */
  /*
   * 作为一个副作用, 中断处理函数通过为中断事件计算时间戳来收集随机信息,
   * 这对 /dev/random 很有用.
   */
  get_randomness(hook->irq);

  /* Add a bit for this interrupt to the process' pending interrupts. When 
   * sending the notification message, this bit map will be magically set
   * as an argument. 
   */
  /*
   * 在进程的未决中断位图中, 为该中断设置一个位. 当发送一个通知消息时,
   * 这个位图将会神奇地作为一个参数传递. ???
   */
  priv(proc_addr(hook->proc_nr))->s_int_pending |= (1 << hook->notify_id);

  /* Build notification message and return. */
  lock_notify(HARDWARE, hook->proc_nr);
  return(hook->policy & IRQ_REENABLE);
}

#endif /* USE_IRQCTL */

