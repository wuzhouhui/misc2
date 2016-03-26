/* This file contains routines for initializing the 8259 interrupt controller:
 *	put_irq_handler: register an interrupt handler
 *	rm_irq_handler: deregister an interrupt handler
 *	intr_handle:	handle a hardware interrupt
 *	intr_init:	initialize the interrupt controller(s)
 */
/*
 * 这个文件包含初始化 8259 中断控制器的例程. 
 *	put_irq_handler: 注册一个中断处理程序;
 *	rm_irq_handler:  注销一个中断处理程序;
 *	intr_handle:	 处理一个硬件中断;
 *	intr_init:	 初始化中断控制器.
 */

#include "kernel.h"
#include "proc.h"
#include <minix/com.h>

#define ICW1_AT         0x11	/* edge triggered, cascade, need ICW4 */
#define ICW1_PC         0x13	/* edge triggered, no cascade, need ICW4 */
#define ICW1_PS         0x19	/* level triggered, cascade, need ICW4 */
#define ICW4_AT_SLAVE   0x01	/* not SFNM, not buffered, normal EOI, 8086 */
#define ICW4_AT_MASTER  0x05	/* not SFNM, not buffered, normal EOI, 8086 */
#define ICW4_PC_SLAVE   0x09	/* not SFNM, buffered, normal EOI, 8086 */
#define ICW4_PC_MASTER  0x0D	/* not SFNM, buffered, normal EOI, 8086 */

#define set_vec(nr, addr)	((void)0)

/*===========================================================================*
 *				intr_init				     *
 *===========================================================================*/
PUBLIC void intr_init(mine)
int mine;
{
/* Initialize the 8259s, finishing with all interrupts disabled.  This is
 * only done in protected mode, in real mode we don't touch the 8259s, but
 * use the BIOS locations instead.  The flag "mine" is set if the 8259s are
 * to be programmed for MINIX, or to be reset to what the BIOS expects.
 */
/*
 * 初始化 8259 芯片, 以禁止所有中断为结束状态. 这些工作只在保护模式中工作,
 * 在实模式中我们不会去碰8259芯片, 而是直接使用 BIOS 的中断. mine 非0, 则
 * 意味着这是为 MINIX 而对 8259 编程, mine 为 0, 则表示这是 BIOS 所期望的.
 *  ???
 */
  int i;

  intr_disable(); // intr_disable() ???

      /* The AT and newer PS/2 have two interrupt controllers, one master,
       * one slaved at IRQ 2.  (We don't have to deal with the PC that
       * has just one controller, because it must run in real mode.)
       */
  /*
   * AT 和 较新的 PS/2 拥有两片中断控制器: 一个作为主控制器, 另一个接在 
   * IRQ2 上作从控制器. (这里不考虑只有一片中断控制器的 PC,
   * 因为在这种情况下只能以实模式运行.
   */
      outb(INT_CTL, machine.ps_mca ? ICW1_PS : ICW1_AT);
      outb(INT_CTLMASK, mine ? IRQ0_VECTOR : BIOS_IRQ0_VEC);
							/* ICW2 for master */
      outb(INT_CTLMASK, (1 << CASCADE_IRQ));		/* ICW3 tells slaves */
      outb(INT_CTLMASK, ICW4_AT_MASTER);
      outb(INT_CTLMASK, ~(1 << CASCADE_IRQ));		/* IRQ 0-7 mask */
      outb(INT2_CTL, machine.ps_mca ? ICW1_PS : ICW1_AT);
      outb(INT2_CTLMASK, mine ? IRQ8_VECTOR : BIOS_IRQ8_VEC);
							/* ICW2 for slave */
      outb(INT2_CTLMASK, CASCADE_IRQ);		/* ICW3 is slave nr */
      outb(INT2_CTLMASK, ICW4_AT_SLAVE);
      outb(INT2_CTLMASK, ~0);				/* IRQ 8-15 mask */

      /* Copy the BIOS vectors from the BIOS to the Minix location, so we
       * can still make BIOS calls without reprogramming the i8259s.
       */
      /* 
       * 将 BIOS 向量从 BIOS 复制到 Minix 的特定位置, 于是我们可以在不用
       * 对 i8259 重编程的情况下调用 BIOS.
       */
      phys_copy(BIOS_VECTOR(0) * 4L, VECTOR(0) * 4L, 8 * 4L);
      // _phys_copy
}

/*===========================================================================*
 *				put_irq_handler				     *
 *===========================================================================*/
// 注册一个中断处理程序
PUBLIC void put_irq_handler(hook, irq, handler)
irq_hook_t *hook;
int irq;
irq_handler_t handler;
{
/* Register an interrupt handler. */
  int id;
  irq_hook_t **line;

  // 首先检查中断请求号的合法性.
  if (irq < 0 || irq >= NR_IRQ_VECTORS)
      panic("invalid call to put_irq_handler", irq);

  line = &irq_handlers[irq];
  id = 1;
  // 遍历与该中断请求号相对应的中断请求钩子链表, 如果已经注册, 则返回;
  // 否则计算链表中元素的个数, 
  while (*line != NULL) {
      if (hook == *line) return;	/* extra initialization */
      line = &(*line)->next;
      id <<= 1;		// 每发发现一个元素,  id 左移一位.
  }
  if (id == 0) panic("Too many handlers for irq", irq);

  // 往链表尾添加一个元素, 即注册一个中断处理程序
  hook->next = NULL;
  hook->handler = handler;
  hook->irq = irq;
  hook->id = id;
  *line = hook;

  // 将 irq_use 位图中与 irq 对应的位开启.
  irq_use |= 1 << irq;
}

/*===========================================================================*
 *				rm_irq_handler				     *
 *===========================================================================*/
// 注销一个中断处理程序
PUBLIC void rm_irq_handler(hook)
irq_hook_t *hook;
{
/* Unregister an interrupt handler. */
  int irq = hook->irq; 
  int id = hook->id;
  irq_hook_t **line;

  // 检查参数的合法性
  if (irq < 0 || irq >= NR_IRQ_VECTORS) 
      panic("invalid call to rm_irq_handler", irq);

  line = &irq_handlers[irq];
  // 将该中断请求函数从对应 的中断请求钩子链表中移除; 如果链表变空, 
  // 则将 irq_use 中与该中断请求号对应 的位关闭.
  while (*line != NULL) {
      if ((*line)->id == id) {
          (*line) = (*line)->next;
          if (! irq_handlers[irq]) irq_use &= ~(1 << irq);
          return;
      }
      line = &(*line)->next;
  }
  /* When the handler is not found, normally return here. */
}

/*===========================================================================*
 *				intr_handle				     *
 *===========================================================================*/
PUBLIC void intr_handle(hook)
irq_hook_t *hook;
{
/* Call the interrupt handlers for an interrupt with the given hook list.
 * The assembly part of the handler has already masked the IRQ, reenabled the
 * controller(s) and enabled interrupts.
 */

  /* Call list of handlers for an IRQ. */
  while (hook != NULL) {
      /* For each handler in the list, mark it active by setting its ID bit,
       * call the function, and unmark it if the function returns true.
       */
	/* 对链表中的每一个中断处理程序, 通过设置它的 ID 位使它成为
	 * 活动状态. 调用中断处理程序, 如果返回值非0, 则关闭 ID 位.
	 */
      irq_actids[hook->irq] |= hook->id;
      if ((*hook->handler)(hook)) irq_actids[hook->irq] &= ~hook->id;
      hook = hook->next;
  }

  // ???
  /* The assembly code will now disable interrupts, unmask the IRQ if and only
   * if all active ID bits are cleared, and restart a process.
   */
  /* 汇编代码现在禁止终端, 解除 IRQ 当且仅当所有的活动 ID 位都已清除,
   * 并且重启进程.
   */
}
