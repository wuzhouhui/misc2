/* This file contains a simple exception handler.  Exceptions in user
 * processes are converted to signals. Exceptions in a kernel task cause
 * a panic.
 */
/*
 * 这个文件包含一个简单的异常处理程序. 用户进程发生的异常会被转换成信号.
 * 内核任务造成的异常会导致恐慌.
 */

#include "kernel.h"
#include <signal.h>
#include "proc.h"

/*===========================================================================*
 *				exception				     *
 *===========================================================================*/
// 异常处理程序, 如果发生异常时运行的是普通进程, 则给进程发送一个信号;
// 如果发生异常时运行的是系统代码, 则崩溃
PUBLIC void exception(vec_nr)
unsigned vec_nr; // 向量号
{
/* An exception or unexpected interrupt has occurred. */

  struct ex_s {
	char *msg;	// 异常消息 
	int signum;	// 信号
	int minprocessor; // 处理器, 8086, 80386, etc.
  };
  static struct ex_s ex_data[] = {
	{ "Divide error", SIGFPE, 86 },
	{ "Debug exception", SIGTRAP, 86 },
	{ "Nonmaskable interrupt", SIGBUS, 86 },
	{ "Breakpoint", SIGEMT, 86 },
	{ "Overflow", SIGFPE, 86 },
	{ "Bounds check", SIGFPE, 186 },
	{ "Invalid opcode", SIGILL, 186 },
	{ "Coprocessor not available", SIGFPE, 186 },
	{ "Double fault", SIGBUS, 286 },
	{ "Copressor segment overrun", SIGSEGV, 286 },
	{ "Invalid TSS", SIGSEGV, 286 },
	{ "Segment not present", SIGSEGV, 286 },
	{ "Stack exception", SIGSEGV, 286 },	/* STACK_FAULT already used */
	{ "General protection", SIGSEGV, 286 },
	{ "Page fault", SIGSEGV, 386 },		/* not close */
	{ NIL_PTR, SIGILL, 0 },			/* probably software trap */
	{ "Coprocessor error", SIGFPE, 386 },
  };
  register struct ex_s *ep;
  struct proc *saved_proc;

  /* Save proc_ptr, because it may be changed by debug statements. */
  /* 保存 proc_ptr, 因为它可能被调试语句更改 */
  saved_proc = proc_ptr;	

  ep = &ex_data[vec_nr];

  // spurious: 伪造的
  if (vec_nr == 2) {		/* spurious NMI on some machines */
	kprintf("got spurious NMI\n");
	return;
  }

  /* If an exception occurs while running a process, the k_reenter variable 
   * will be zero. Exceptions in interrupt handlers or system traps will make 
   * k_reenter larger than zero.
   */
  /*
   * 如果发生异常时运行的是进程, 那么 k_reenter 的值将会是0. 在中断处理程序
   * 或系统陷阱中发生异常, 此时 k_reenter 的值将会大于0.
   */
  // 如果发生异常时运行的是用户进程, 并且处在用户态, 向发生异常的进程发送
  // 异常对应的信号
  if (k_reenter == 0 && ! iskernelp(saved_proc)) {
	cause_sig(proc_nr(saved_proc), ep->signum);
	return;
  }

  /* Exception in system code. This is not supposed to happen. */
  // 如果发生的异常是一般保护性异常, 或者机器的处理器型号低于
  // 异常的处理器型号
  if (ep->msg == NIL_PTR || machine.processor < ep->minprocessor)
	kprintf("\nIntel-reserved exception %d\n", vec_nr);
  else
	kprintf("\n%s\n", ep->msg);
  // 打印内核重入计数器值
  kprintf("k_reenter = %d ", k_reenter);
  // 打印进程号与进程名
  kprintf("process %d (%s), ", proc_nr(saved_proc), saved_proc->p_name);
  // 打印进程的代码段寄存器与程序计数器(IP)的值.
  kprintf("pc = %u:0x%x", (unsigned) saved_proc->p_reg.cs,
  (unsigned) saved_proc->p_reg.pc);

  panic("exception in a kernel task", NO_NUM);
}

