#ifndef _MINIX_SYS_CONFIG_H
#define _MINIX_SYS_CONFIG_H 1

/* This is a modified sys_config.h for compiling a small Minix system 
 * with only the options described in the text, Operating Systems Design and 
 * Implementation, 3rd edition. See the sys_config.h in the full 
 * source code directory for information on alternatives omitted here.
 */

/*===========================================================================*
 *		This section contains user-settable parameters		     *
 *===========================================================================*/
#define _MINIX_MACHINE       _MACHINE_IBM_PC

#define _MACHINE_IBM_PC             1	/* any  8088 or 80x86-based system */

/* Word size in bytes (a constant equal to sizeof(int)). */
#if __ACK__ || __GNUC__
#define _WORD_SIZE	_EM_WSIZE
#define _PTR_SIZE	_EM_WSIZE
#endif

#define _NR_PROCS	64
#define _NR_SYS_PROCS	32

/* Set the CHIP type based on the machine selected. The symbol CHIP is actually
 * indicative of more than just the CPU.  For example, machines for which
 * CHIP == INTEL are expected to have 8259A interrrupt controllers and the
 * other properties of IBM PC/XT/AT/386 types machines in general. */
#define _CHIP_INTEL     1	/* CHIP type for PC, XT, AT, 386 and clones */

/* Set the FP_FORMAT type based on the machine selected, either hw or sw    */
#define _FP_NONE	0	/* no floating point support                */
#define _FP_IEEE	1	/* conform IEEE floating point standard     */

#define _MINIX_CHIP          _CHIP_INTEL

#define _MINIX_FP_FORMAT   _FP_NONE

#ifndef _MINIX_MACHINE
error "In <minix/sys_config.h> please define _MINIX_MACHINE"
#endif

#ifndef _MINIX_CHIP
error "In <minix/sys_config.h> please define _MINIX_MACHINE to have a legal value"
#endif

#if (_MINIX_MACHINE == 0)
error "_MINIX_MACHINE has incorrect value (0)"
#endif

#endif /* _MINIX_SYS_CONFIG_H */


