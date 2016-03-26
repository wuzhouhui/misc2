#ifndef _TYPE_H
#define _TYPE_H

#ifndef _MINIX_SYS_CONFIG_H
#include <minix/sys_config.h>
#endif

#ifndef _TYPES_H
#include <sys/types.h>
#endif

/* Type definitions. */
// 1 click equals 1024 bytes
typedef unsigned int vir_clicks; 	/*  virtual addr/length in clicks */
typedef unsigned long phys_bytes;	/* physical addr/length in bytes */
typedef unsigned int phys_clicks;	/* physical addr/length in clicks */

#if (_MINIX_CHIP == _CHIP_INTEL)
typedef unsigned int vir_bytes;	/* virtual addresses and lengths in bytes */
#endif

#if (_MINIX_CHIP == _CHIP_M68000)
typedef unsigned long vir_bytes;/* virtual addresses and lengths in bytes */
#endif

#if (_MINIX_CHIP == _CHIP_SPARC)
typedef unsigned long vir_bytes;/* virtual addresses and lengths in bytes */
#endif

/* Memory map for local text, stack, data segments. */
/* 局部代码段, 堆栈段, 数据段的内存映射 */
struct mem_map {
// 虚拟地址, 应该也就是线性地址
  vir_clicks mem_vir;		/* virtual address */
  // 物理地址, 以 click 为单位
  phys_clicks mem_phys;		/* physical address */
  // 长度, 以 click 为单位.
  vir_clicks mem_len;		/* length */
};

/* Memory map for remote memory areas, e.g., for the RAM disk. */
/* 远程内存区(如 RAM 盘)的内存映射. */
struct far_mem {
// 使用标志
  int in_use;			/* entry in use, unless zero */
  phys_clicks mem_phys;		/* physical address */
  vir_clicks mem_len;		/* length */
};

/* Structure for virtual copying by means of a vector with requests. */
/* 虚拟地址复制的结构, 依靠一个带请求的向量 */
struct vir_addr {
  int proc_nr;		// 进程号
  int segment;		// 段
  vir_bytes offset;	// 段内偏移
};

#define phys_cp_req vir_cp_req 
// 描述 复制请求的结构体.
struct vir_cp_req {
  struct vir_addr src;
  struct vir_addr dst;
  phys_bytes count;
};

typedef struct {
  vir_bytes iov_addr;		/* address of an I/O buffer */
  vir_bytes iov_size;		/* sizeof an I/O buffer */
} iovec_t;

/* PM passes the address of a structure of this type to KERNEL when
 * sys_sendsig() is invoked as part of the signal catching mechanism.
 * The structure contain all the information that KERNEL needs to build
 * the signal stack.
 */
/*
 * 当 sys_sendsig() 作为信号捕获机制的一部分被调用时, PM 会将这种类型的
 * 一个指针传递给 KERNEL. 该结构包含 KERNEL 用来构造信号栈的所有信息.
 */
struct sigmsg {
  int sm_signo;			/* signal number being caught */
  unsigned long sm_mask;	/* mask to restore when handler returns */
  vir_bytes sm_sighandler;	/* address of handler */
  vir_bytes sm_sigreturn;	/* address of _sigreturn in C library */
  vir_bytes sm_stkptr;		/* user stack pointer */
};

/* This is used to obtain system information through SYS_GETINFO. */
/* 通过 SYS_GETINFO 获取系统信息 */
struct kinfo {
// 内核代码段基地址
  phys_bytes code_base;		/* base of kernel code */
  phys_bytes code_size;		
  // 内核数据段基地址
  phys_bytes data_base;		/* base of kernel data */
  phys_bytes data_size;
  // 进程表地址, 虚拟地址.
  vir_bytes proc_addr;		/* virtual address of process table */
  // 内核在内存中的起始地址.
  phys_bytes kmem_base;		/* kernel memory layout (/dev/kmem) */
  // 内核占据的内存大小.
  phys_bytes kmem_size;
  phys_bytes bootdev_base;	/* boot device from boot image (/dev/boot) */
  phys_bytes bootdev_size;
  phys_bytes bootdev_mem;
  // 监视器会将参数置于该地址开始的内存单元.
  phys_bytes params_base;	/* parameters passed by boot monitor */
  // 内核信息结构所能容纳的参数数量
  phys_bytes params_size;
  // 用户进程数
  int nr_procs;			/* number of user processes */
  // 内核任务数
  int nr_tasks;			/* number of kernel tasks */
  // 内核发行号
  char release[6];		/* kernel release number */
  // 内核版本号
  char version[6];		/* kernel version number */
  int relocking;		/* relocking check (for debugging) */
};

// 跟机器有关的数据
struct machine {
  int pc_at;		// ibm pc at 兼容机
  int ps_mca;
  int processor;	// 处理器
  int protected;	// 保护模式
  int vdu_ega;
  int vdu_vga;
};

#endif /* _TYPE_H */
