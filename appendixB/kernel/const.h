/* General macros and constants used by the kernel. */
#ifndef CONST_H
#define CONST_H

#include <ibm/interrupt.h>	/* interrupt numbers and hardware vectors */
#include <ibm/ports.h>		/* port addresses and magic numbers */
#include <ibm/bios.h>		/* BIOS addresses, sizes and magic numbers */
#include <ibm/cpu.h>		/* BIOS addresses, sizes and magic numbers */
#include <minix/config.h>
#include "config.h"

/* To translate an address in kernel space to a physical address.  This is
 * the same as umap_local(proc_ptr, D, vir, sizeof(*vir)), but less costly.
 */
/*
 * 将内核空间中的一个地址转换成物理地址. 与 umap_local(proc_ptr, D, vir,
 * sizeof(*vir)) 相同, 但是开销更小
 */
// 把虚拟地址加上内核的数据基地址, 就得到了物理地址.
#define vir2phys(vir)	(kinfo.data_base + (vir_bytes) (vir))

/* Map a process number to a privilege structure id. */
/* 将一个进程号映射到它的特权级结构 id */
// +1 是因为特权级结构 0 被预留给了普通用户进程, 因此这里的 n 也不能是
// 普通用户进程, 可以是系统服务进程.
#define s_nr_to_id(n)	(NR_TASKS + (n) + 1)

/* Translate a pointer to a field in a structure to a pointer to the structure
 * itself. So it translates '&struct_ptr->field' back to 'struct_ptr'.
 */
#define structof(type, field, ptr) \
	((type *) (((char *) (ptr)) - offsetof(type, field)))

/* Constants used in virtual_copy(). Values must be 0 and 1, respectively. */
#define _SRC_	0
#define _DST_	1

/* Number of random sources */
/* 随机源的号码 ??? */
#define RANDOM_SOURCES	16

/* Constants and macros for bit map manipulation. */
// 位图块的二进制位数
#define BITCHUNK_BITS   (sizeof(bitchunk_t) * CHAR_BIT)
// 计算 nr_bits 个二进制位所需要的位图块数, 上取整
#define BITMAP_CHUNKS(nr_bits) (((nr_bits)+BITCHUNK_BITS-1)/BITCHUNK_BITS)  
#define MAP_CHUNK(map,bit) (map)[((bit)/BITCHUNK_BITS)]
// 计算 bit 在位图中的偏移量
#define CHUNK_OFFSET(bit) ((bit)%BITCHUNK_BITS) // )
#define GET_BIT(map,bit) ( MAP_CHUNK(map,bit) & (1 << CHUNK_OFFSET(bit) )
#define SET_BIT(map,bit) ( MAP_CHUNK(map,bit) |= (1 << CHUNK_OFFSET(bit) )
#define UNSET_BIT(map,bit) ( MAP_CHUNK(map,bit) &= ~(1 << CHUNK_OFFSET(bit) )

#define get_sys_bit(map,bit) \
	( MAP_CHUNK(map.chunk,bit) & (1 << CHUNK_OFFSET(bit) )
  // 将位图中第 bit(从0算起) 位设置为1
#define set_sys_bit(map,bit) \
	( MAP_CHUNK(map.chunk,bit) |= (1 << CHUNK_OFFSET(bit) )
#define unset_sys_bit(map,bit) \
	( MAP_CHUNK(map.chunk,bit) &= ~(1 << CHUNK_OFFSET(bit) )
#define NR_SYS_CHUNKS	BITMAP_CHUNKS(NR_SYS_PROCS)

/* Program stack words and masks. */
#define INIT_PSW      0x0200	/* initial psw */
// 内核任务的初始 processor status word. i/o 特权级为1.
#define INIT_TASK_PSW 0x1200	/* initial psw for tasks (with IOPL 1) */
#define TRACEBIT      0x0100	/* OR this with psw in proc[] for tracing */
#define SETPSW(rp, new)		/* permits only certain bits to be set */ \
	((rp)->p_reg.psw = (rp)->p_reg.psw & ~0xCD5 | (new) & 0xCD5)
#define IF_MASK 0x00000200
#define IOPL_MASK 0x003000

/* Disable/ enable hardware interrupts. The parameters of lock() and unlock()
 * are used when debugging is enabled. See debug.h for more information.
 */
  // 加锁(关中断) 与解锁(开中断). 加锁与解锁的参数用于调试
#define lock(c, v)	intr_disable(); // ???
#define unlock(c)	intr_enable();  // ???

/* Sizes of memory tables. The boot monitor distinguishes three memory areas, 
 * namely low mem below 1M, 1M-16M, and mem after 16M. More chunks are needed
 * for DOS MINIX.
 */
#define NR_MEMS            8	

#endif /* CONST_H */





