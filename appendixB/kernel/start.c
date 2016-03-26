/* This file contains the C startup code for Minix on Intel processors.
 * It cooperates with mpx.s to set up a good environment for main().
 *
 * This code runs in real mode for a 16 bit kernel and may have to switch
 * to protected mode for a 286.
 * For a 32 bit kernel this already runs in protected mode, but the selectors
 * are still those given by the BIOS with interrupts disabled, so the
 * descriptors need to be reloaded and interrupt descriptors made.
 */
/*
 * 这个文件包含 Minix 在 英特尔处理器上的 C 启动代码. 它和 mpx.s 合作
 * 为 main() 构造一个合适的环境.
 *
 * 这里的代码对于 16 位的内核会在 实模式下运行, 所以可能会切换到 保护模式,
 * 如果在 286 中.
 * 对于 32 位的内核, 这里的代码已经运行在保护模式中, 但是段选择符(由 BIOS 
 * 给出的) 仍然是禁止中断的, 描述符需要重新加载, ???
 */

#include "kernel.h"
#include "protect.h"
#include "proc.h"
#include <stdlib.h>
#include <string.h>

FORWARD _PROTOTYPE( char *get_value, (_CONST char *params, _CONST char *key));
/*===========================================================================*
 *				cstart					     *
 *===========================================================================*/
PUBLIC void cstart(cs, ds, mds, parmoff, parmsize)
U16_t cs, ds;			/* kernel code and data segment */
U16_t mds;			/* monitor data segment */
U16_t parmoff, parmsize;	/* boot parameters offset and length */
{
/* Perform system initializations prior to calling main(). Most settings are
 * determined with help of the environment strings passed by MINIX' loader.
 */
/*
 * 在调用 main() 初始化系统. 在大多数设置是在环境字符串的帮助下完成的,
 * 这些环境字符串由 MINIX 的装载器传递而来.
 */
  char params[128*sizeof(char *)];		/* boot monitor parameters */
  register char *value;				/* value in key=value pair */
  extern int etext, end;	// 应该由链接器设置, 同 'end' of linux-0.11

  /* Decide if mode is protected; 386 or higher implies protected mode.
   * This must be done first, because it is needed for, e.g., seg2phys().
   * For 286 machines we cannot decide on protected mode, yet. This is 
   * done below. 
   */
  /*
   * 判断当前模式是否是保护模式; 386 或更高隐含是保护模式. 这个必须要首先
   * 完成, 因为许多函数依赖于此, 例如 seg2phys(). 对于 286 我们不能判断
   * 是否已经是保护模式, 所以才需要下面这三行代码.
   */
  // 如果字长不是 2 个字节, 说明当前是保护模式. 32 位的保护模式字长应该是
  // 32 位.
#if _WORD_SIZE != 2
  machine.protected = 1;	
#endif

  /* Record where the kernel and the monitor are. */
  /* 记录内核与监视器的位置 */
  kinfo.code_base = seg2phys(cs);
  kinfo.code_size = (phys_bytes) &etext;	/* size of code segment */
  kinfo.data_base = seg2phys(ds);
  kinfo.data_size = (phys_bytes) &end;		/* size of data segment */

  /* Initialize protected mode descriptors. */
  /* 初始化保护模式的描述符 */
  prot_init();

  /* Copy the boot parameters to the local buffer. */
  /* 将引导参数复制到本地缓冲区 */
  kinfo.params_base = seg2phys(mds) + parmoff;
  // 这里减 2 是因为: 1) 条目的结尾 '\0'; 2) 下一个条目为 0 标记表格
  // 的结束.
  kinfo.params_size = MIN(parmsize,sizeof(params)-2); // why minus 2??
  // _phys_copy
  phys_copy(kinfo.params_base, vir2phys(params), kinfo.params_size);

  /* Record miscellaneous information for user-space servers. */
  kinfo.nr_procs = NR_PROCS;
  kinfo.nr_tasks = NR_TASKS;
  strncpy(kinfo.release, OS_RELEASE, sizeof(kinfo.release));
  kinfo.release[sizeof(kinfo.release)-1] = '\0';
  strncpy(kinfo.version, OS_VERSION, sizeof(kinfo.version));
  kinfo.version[sizeof(kinfo.version)-1] = '\0';
  kinfo.proc_addr = (vir_bytes) proc;
  kinfo.kmem_base = vir2phys(0);
  kinfo.kmem_size = (phys_bytes) &end;	

  /* Processor?  86, 186, 286, 386, ... 
   * Decide if mode is protected for older machines. 
   */
  machine.processor=atoi(get_value(params, "processor")); 
#if _WORD_SIZE == 2
  machine.protected = machine.processor >= 286;		
#endif
  if (! machine.protected) mon_return = 0;

  /* XT, AT or MCA bus? */
  value = get_value(params, "bus");
  if (value == NIL_PTR || strcmp(value, "at") == 0) {
      machine.pc_at = TRUE;			/* PC-AT compatible hardware */
  } else if (strcmp(value, "mca") == 0) {
      machine.pc_at = machine.ps_mca = TRUE;	/* PS/2 with micro channel */
  }

  /* Type of VDU: */
  // vdu: 图像文字显示屏
  // ega: 增加形图形适配器
  // vga: 视频图形阵列
  value = get_value(params, "video");		/* EGA or VGA video unit */
  if (strcmp(value, "ega") == 0) machine.vdu_ega = TRUE;
  if (strcmp(value, "vga") == 0) machine.vdu_vga = machine.vdu_ega = TRUE;

  /* Return to assembler code to switch to protected mode (if 286), 
   * reload selectors and call main().
   */
}

/*===========================================================================*
 *				get_value				     *
 *===========================================================================*/
// 健值查找
PRIVATE char *get_value(params, name)
_CONST char *params;				/* boot monitor parameters */
_CONST char *name;				/* key to look up */
{
/* Get environment value - kernel version of getenv to avoid setting up the
 * usual environment array.
 */
/*
 * 获取环境值 - 内核版的 getenv 避免了设置通常的环境数组.
 */
  register _CONST char *namep;
  register char *envp;

  for (envp = (char *) params; *envp != 0;) {
	for (namep = name; *namep != 0 && *namep == *envp; namep++, envp++)
		;
	if (*namep == '\0' && *envp == '=') return(envp + 1);
	while (*envp++ != 0)
		;
  }
  return(NIL_PTR);
}
