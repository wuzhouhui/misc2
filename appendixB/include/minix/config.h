#ifndef _CONFIG_H
#define _CONFIG_H

/* Minix release and version numbers. */
#define OS_RELEASE "3"
#define OS_VERSION "1.0"

/* This file sets configuration parameters for the MINIX kernel, FS, and PM.
 * It is divided up into two main sections.  The first section contains
 * user-settable parameters.  In the second section, various internal system
 * parameters are set based on the user-settable parameters.
 *
 * Parts of config.h have been moved to sys_config.h, which can be included
 * by other include files that wish to get at the configuration data, but
 * don't want to pollute the users namespace. Some editable values have
 * gone there.
 *
 * This is a modified version of config.h for compiling a small Minix system
 * with only the options described in the text, Operating Systems Design and
 * Implementation, 3rd edition. See the version of config.h in the full 
 * source code directory for information on alternatives omitted here.
 */

/* The MACHINE (called _MINIX_MACHINE) setting can be done
 * in <minix/machine.h>.
 */
#include <minix/sys_config.h>

#define MACHINE      _MINIX_MACHINE

#define IBM_PC       _MACHINE_IBM_PC
#define SUN_4        _MACHINE_SUN_4
#define SUN_4_60     _MACHINE_SUN_4_60
#define ATARI        _MACHINE_ATARI
#define MACINTOSH    _MACHINE_MACINTOSH

/* Number of slots in the process table for non-kernel processes. The number
 * of system processes defines how many processes with special privileges 
 * there can be. User processes share the same properties and count for one. 
 *
 * These can be changed in sys_config.h.
 */
/*
 * 非内核进程在进程表中的项数. 系统进程数定义了可以有多少个进程带有特殊的
 * 特权级. 用户进程共享相同的属性, 并且只算一个.
 *
 * 这些值可以在 sys_config.h 中修改.
 */
// 非系统进程数
#define NR_PROCS 	  _NR_PROCS 
// 系统进程数
#define NR_SYS_PROCS      _NR_SYS_PROCS

#define NR_BUFS	128
#define NR_BUF_HASH 128

/* Number of controller tasks (/dev/cN device classes). */
#define NR_CTRLRS          2

/* Enable or disable the second level file system cache on the RAM disk. */
#define ENABLE_CACHE2      0

/* Enable or disable swapping processes to disk. */
#define ENABLE_SWAP	   0 

/* Include or exclude an image of /dev/boot in the boot image. 
 * Please update the makefile in /usr/src/tools/ as well.
 */
#define ENABLE_BOOTDEV	   0	/* load image of /dev/boot at boot time */

/* DMA_SECTORS may be increased to speed up DMA based drivers. */
#define DMA_SECTORS        1	/* DMA buffer size (must be >= 1) */

/* Include or exclude backwards compatibility code. */
#define ENABLE_BINCOMPAT   0	/* for binaries using obsolete calls */
#define ENABLE_SRCCOMPAT   0	/* for sources using obsolete calls */

/* Which process should receive diagnostics from the kernel and system? 
 * Directly sending it to TTY only displays the output. Sending it to the
 * log driver will cause the diagnostics to be buffered and displayed.
 */
/*
 * 该由哪一个进程接收内核或系统的诊断信息? 直接将消息发送给 TTY 只能
 * 显示信息. 发送给日志驱动程序会导致诊断信息被缓冲, 然后才会显示.
 */
#define OUTPUT_PROC_NR	LOG_PROC_NR	/* TTY_PROC_NR or LOG_PROC_NR */

/* NR_CONS, NR_RS_LINES, and NR_PTYS determine the number of terminals the
 * system can handle.
 */
#define NR_CONS            4	/* # system consoles (1 to 8) */
#define	NR_RS_LINES	   0	/* # rs232 terminals (0 to 4) */
#define	NR_PTYS		   0	/* # pseudo terminals (0 to 64) */

/*===========================================================================*
 *	There are no user-settable parameters after this line		     *
 *===========================================================================*/
/* Set the CHIP type based on the machine selected. The symbol CHIP is actually
 * indicative of more than just the CPU.  For example, machines for which
 * CHIP == INTEL are expected to have 8259A interrrupt controllers and the
 * other properties of IBM PC/XT/AT/386 types machines in general. */
#define INTEL             _CHIP_INTEL	/* CHIP type for PC, XT, AT, 386 and clones */
#define M68000            _CHIP_M68000	/* CHIP type for Atari, Amiga, Macintosh    */
#define SPARC             _CHIP_SPARC	/* CHIP type for SUN-4 (e.g. SPARCstation)  */

/* Set the FP_FORMAT type based on the machine selected, either hw or sw    */
#define FP_NONE	 _FP_NONE	/* no floating point support                */
#define FP_IEEE	 _FP_IEEE	/* conform IEEE floating point standard     */

/* _MINIX_CHIP is defined in sys_config.h. */
#define CHIP	_MINIX_CHIP

/* _MINIX_FP_FORMAT is defined in sys_config.h. */
#define FP_FORMAT	_MINIX_FP_FORMAT

/* _ASKDEV and _FASTLOAD are defined in sys_config.h. */
#define ASKDEV _ASKDEV
#define FASTLOAD _FASTLOAD

#endif /* _CONFIG_H */
