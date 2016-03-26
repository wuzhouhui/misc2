/* This file contains code for initialization of protected mode, to initialize
 * code and data segment descriptors, and to initialize global descriptors
 * for local descriptors in the process table.
 */
/*
 * 这个文件含有保护模式的初始化代码, 包括初始化数据段与代码段描述符, 为局部
 * 描述符表初始化全局描述符表.
 */

#include "kernel.h"
#include "proc.h"
#include "protect.h"

#define INT_GATE_TYPE	(INT_286_GATE | DESC_386_BIT)
#define TSS_TYPE	(AVL_286_TSS  | DESC_386_BIT)

// 段描述符, 仅包含段限长与段基地址部分.
struct desctableptr_s {
  // 段限长
  char limit[sizeof(u16_t)];
  // 段基地址
  char base[sizeof(u32_t)];		/* really u24_t + pad for 286 */
};

// 门描述符
struct gatedesc_s {
  // 段中偏移值, 低 16 位.
  u16_t offset_low;
  // 段选择符
  u16_t selector;
  // etc.
  u8_t pad;			/* |000|XXXXX| ig & trpg, |XXXXXXXX| task g */
  // 存在位, 描述符特权级, 描述符类型
  u8_t p_dpl_type;		/* |P|DL|0|TYPE| */
  // 段中偏移值, 高 16 位.
  u16_t offset_high;
};

struct tss_s {
  reg_t backlink;
  reg_t sp0;                    /* stack pointer to use during interrupt */
  reg_t ss0;                    /*   "   segment  "  "    "        "     */
  reg_t sp1;
  reg_t ss1;
  reg_t sp2;
  reg_t ss2;
  reg_t cr3;
  reg_t ip;
  reg_t flags;
  reg_t ax;
  reg_t cx;
  reg_t dx;
  reg_t bx;
  reg_t sp;
  reg_t bp;
  reg_t si;
  reg_t di;
  reg_t es;
  reg_t cs;
  reg_t ss;
  reg_t ds;
  reg_t fs;
  reg_t gs;
  reg_t ldt;
  u16_t trap;
  u16_t iobase;
/* u8_t iomap[0]; */
};

// 全局描述符表
PUBLIC struct segdesc_s gdt[GDT_SIZE];		/* used in klib.s and mpx.s */
// 中断描述符表
PRIVATE struct gatedesc_s idt[IDT_SIZE];	/* zero-init so none present */
// 内核用的 TSS. (应该是内核态时的 TSS)
PUBLIC struct tss_s tss;			/* zero init */

FORWARD _PROTOTYPE( void int_gate, (unsigned vec_nr, vir_bytes offset,
		unsigned dpl_type) );
FORWARD _PROTOTYPE( void sdesc, (struct segdesc_s *segdp, phys_bytes base,
		vir_bytes size) );

/*===========================================================================*
 *				prot_init				     *
 *===========================================================================*/
PUBLIC void prot_init()
{
/* Set up tables for protected mode.
 * All GDT slots are allocated at compile time.
 */
/*
 * 为保护模式构造表格. 所有的 GDT 槽都在编译时分配.
 */
  struct gate_table_s *gtp;
  struct desctableptr_s *dtp;
  unsigned ldt_index;
  register struct proc *rp;

  static struct gate_table_s {
	_PROTOTYPE( void (*gate), (void) ); // 中断处理函数
	unsigned char vec_nr;		    // 中断向量号
	unsigned char privilege;	    // 特权级
  }
  gate_table[] = {
	/* 中断处理函数  中断向量号     特权级 */
	{ divide_error, DIVIDE_VECTOR, INTR_PRIVILEGE },
	{ single_step_exception, DEBUG_VECTOR, INTR_PRIVILEGE },
	{ nmi, NMI_VECTOR, INTR_PRIVILEGE },
	{ breakpoint_exception, BREAKPOINT_VECTOR, USER_PRIVILEGE },
	{ overflow, OVERFLOW_VECTOR, USER_PRIVILEGE },
	{ _bounds_check, BOUNDS_VECTOR, INTR_PRIVILEGE },
	{ inval_opcode, INVAL_OP_VECTOR, INTR_PRIVILEGE },
	{ copr_not_available, COPROC_NOT_VECTOR, INTR_PRIVILEGE },
	{ double_fault, DOUBLE_FAULT_VECTOR, INTR_PRIVILEGE },
	{ copr_seg_overrun, COPROC_SEG_VECTOR, INTR_PRIVILEGE },
	{ inval_tss, INVAL_TSS_VECTOR, INTR_PRIVILEGE },
	{ segment_not_present, SEG_NOT_VECTOR, INTR_PRIVILEGE },
	{ stack_exception, STACK_FAULT_VECTOR, INTR_PRIVILEGE },
	{ general_protection, PROTECTION_VECTOR, INTR_PRIVILEGE },
	{ page_fault, PAGE_FAULT_VECTOR, INTR_PRIVILEGE },
	{ copr_error, COPROC_ERR_VECTOR, INTR_PRIVILEGE },
	// 来自中断控制器(8259)的中断请求
	{ hwint00, VECTOR( 0), INTR_PRIVILEGE },
	{ hwint01, VECTOR( 1), INTR_PRIVILEGE },
	{ hwint02, VECTOR( 2), INTR_PRIVILEGE },
	{ hwint03, VECTOR( 3), INTR_PRIVILEGE },
	{ hwint04, VECTOR( 4), INTR_PRIVILEGE },
	{ hwint05, VECTOR( 5), INTR_PRIVILEGE },
	{ hwint06, VECTOR( 6), INTR_PRIVILEGE },
	{ hwint07, VECTOR( 7), INTR_PRIVILEGE },
	{ hwint08, VECTOR( 8), INTR_PRIVILEGE },
	{ hwint09, VECTOR( 9), INTR_PRIVILEGE },
	{ hwint10, VECTOR(10), INTR_PRIVILEGE },
	{ hwint11, VECTOR(11), INTR_PRIVILEGE },
	{ hwint12, VECTOR(12), INTR_PRIVILEGE },
	{ hwint13, VECTOR(13), INTR_PRIVILEGE },
	{ hwint14, VECTOR(14), INTR_PRIVILEGE },
	{ hwint15, VECTOR(15), INTR_PRIVILEGE },

	// 系统调用
	{ s_call, SYS386_VECTOR, USER_PRIVILEGE },	/* 386 system call */
	// 以特权级 0 的状态来调用函数
	{ level0_call, LEVEL0_VECTOR, TASK_PRIVILEGE },
  };

  /*
   *	  +---------------------+
   *	F | LDT(1st)		| 第 1 个 LDT 描述符
   *	  +---------------------+
   *		   .
   *		   .
   *		   .
   *	  +---------------------+
   *	A | kernel ES(286)	| ???
   *	  +---------------------+
   *	9 | kernel DS(286)	| ???
   *	  +---------------------+
   *	8 | kernel TSS		| 内核的 TSS 描述符 
   *	  +---------------------+
   *		   .
   *		   .
   *		   .
   *	  +---------------------+
   *	6 | kernel CS		| 内核代码段
   *	  +---------------------+
   *		   .
   *		   .
   *		   .
   *	  +---------------------+
   *	4 | kernel ES		| ES 段, 段基地址为 0, 段限长 4 G.
   *	  +---------------------+
   *	3 | kernel DS		| 内核数据段
   *	  +---------------------+
   *	2 | IDT			| 中段描述符表的段描述符
   *	  +---------------------+
   *	1 | GDT			| 全局描述表的段描述符
   *	  +---------------------+
   *	0 | NULL		| 不用
   *	  +---------------------+
   *		    GDT
   */
  /* Build gdt and idt pointers in GDT where the BIOS expects them. */
  // 全局描述表的段描述符, 在 gdt 中占用一项
  dtp= (struct desctableptr_s *) &gdt[GDT_INDEX];
  * (u16_t *) dtp->limit = (sizeof gdt) - 1;
  * (u32_t *) dtp->base = vir2phys(gdt);

  // 中断描述符表的段描述符, 在 gdt 中占用一项.
  dtp= (struct desctableptr_s *) &gdt[IDT_INDEX];
  * (u16_t *) dtp->limit = (sizeof idt) - 1;
  * (u32_t *) dtp->base = vir2phys(idt);

  /* Build segment descriptors for tasks and interrupt handlers. */
  /* 为任务与中断处理程序建立段描述符 */
  init_codeseg(&gdt[CS_INDEX],
  	 kinfo.code_base, kinfo.code_size, INTR_PRIVILEGE);
  init_dataseg(&gdt[DS_INDEX],
  	 kinfo.data_base, kinfo.data_size, INTR_PRIVILEGE);
  // 在设置段描述符的时候, 会将传入的段限长参数(这里是 0)减一,
  // 然后再设置段限长, 这里将 0 减 1 后就变成 -1(0xFF...F)???
  // 恒等映射 4G 线性地址空间到物理地址空间.
  init_dataseg(&gdt[ES_INDEX], 0L, 0, TASK_PRIVILEGE);

  /* Build scratch descriptors for functions in klib88. */
  /* 为 klib88 中的函数构造 "擦除"(???) 描述符.  */
  init_dataseg(&gdt[DS_286_INDEX], 0L, 0, TASK_PRIVILEGE);
  init_dataseg(&gdt[ES_286_INDEX], 0L, 0, TASK_PRIVILEGE);

  /* Build local descriptors in GDT for LDT's in process table.
   * The LDT's are allocated at compile time in the process table, and
   * initialized whenever a process' map is initialized or changed.
   */
  /*
   * 为进程表中的 ldt 字段, 在 GDT 中构造 LDT 描述符. 进程的 ldt 字段 
   * 在编译时分配存储空间, 当进程的映射改变或初始化时, 重新初始化.
   */
  for (rp = BEG_PROC_ADDR, ldt_index = FIRST_LDT_INDEX;
       rp < END_PROC_ADDR; ++rp, ldt_index++) {
	  // why always 'INTR_PRIVILEGE' ???
	init_dataseg(&gdt[ldt_index], vir2phys(rp->p_ldt),
				     sizeof(rp->p_ldt), INTR_PRIVILEGE);
	gdt[ldt_index].access = PRESENT | LDT;
	rp->p_ldt_sel = ldt_index * DESC_SIZE;
  }

  /* Build main TSS.
   * This is used only to record the stack pointer to be used after an
   * interrupt.
   * The pointer is set up so that an interrupt automatically saves the
   * current process's registers ip:cs:f:sp:ss in the correct slots in the
   * process table.
   */
  /*
   * 构造主要的 TSS.
   * 这主要用于记录栈指针, 当一个中断发生后. 指针被设置, 于是中断可以自动
   * 地保存当前进程的寄存器 ip:cs:f:sp:ss, 保存在进程表的适当的表槽中.
   */
  tss.ss0 = DS_SELECTOR;
  init_dataseg(&gdt[TSS_INDEX], vir2phys(&tss), sizeof(tss), INTR_PRIVILEGE);
  // 上面调用了数据段描述符初始化函数, 因此这里要重新设置专属于 TSS 的字段.
  gdt[TSS_INDEX].access = PRESENT | (INTR_PRIVILEGE << DPL_SHIFT) | TSS_TYPE;

  /* Build descriptors for interrupt gates in IDT. */
  /* 为 IDT 中的中断门设置描述符 */
  for (gtp = &gate_table[0];
       gtp < &gate_table[sizeof gate_table / sizeof gate_table[0]]; ++gtp) {
	int_gate(gtp->vec_nr, (vir_bytes) gtp->gate,
		 PRESENT | INT_GATE_TYPE | (gtp->privilege << DPL_SHIFT));
  }

  /* Complete building of main TSS. */
  // ??? why not tss.iobase = 0
  tss.iobase = sizeof tss;	/* empty i/o permissions map */
}

/*===========================================================================*
 *				init_codeseg				     *
 *===========================================================================*/
// 初始化代码段描述符.
PUBLIC void init_codeseg(segdp, base, size, privilege)
register struct segdesc_s *segdp;
phys_bytes base;
vir_bytes size;
int privilege;
{
/* Build descriptor for a code segment. */
/* 为一个代码段构建段描述符 */
  sdesc(segdp, base, size);
  // 设置段描述符的特权级, 存在位, 段类型, 段属性等
  segdp->access = (privilege << DPL_SHIFT)
	        | (PRESENT | SEGMENT | EXECUTABLE | READABLE);
		/* CONFORMING = 0, ACCESSED = 0 */
}

/*===========================================================================*
 *				init_dataseg				     *
 *===========================================================================*/
// 初始化数据段描述符
PUBLIC void init_dataseg(segdp, base, size, privilege)
register struct segdesc_s *segdp;
phys_bytes base;
vir_bytes size;
int privilege;
{
/* Build descriptor for a data segment. */
  sdesc(segdp, base, size);
  segdp->access = (privilege << DPL_SHIFT) | (PRESENT | SEGMENT | WRITEABLE);
		/* EXECUTABLE = 0, EXPAND_DOWN = 0, ACCESSED = 0 */
}

/*===========================================================================*
 *				sdesc					     *
 *===========================================================================*/
PRIVATE void sdesc(segdp, base, size)
register struct segdesc_s *segdp;
phys_bytes base;
vir_bytes size;
{
/* Fill in the size fields (base, limit and granularity) of a descriptor. */
/* 填充描述符的 size 域(基地址, 段限长 和 粒度 */
  segdp->base_low = base;
  segdp->base_middle = base >> BASE_MIDDLE_SHIFT;
  segdp->base_high = base >> BASE_HIGH_SHIFT;

  // 段限长指的是段的最后一个有效地址, 因此是(段长度 - 1)
  --size;			/* convert to a limit, 0 size means 4G */
  // 如果段限长超过 粒度是字节时所能允许的最大段限长, 则将段限长换算成以
  // 页单位, 然后再相应地设置段限长粒度与段限长的高 4 位
  if (size > BYTE_GRAN_MAX) {
	segdp->limit_low = size >> PAGE_GRAN_SHIFT;
	segdp->granularity = GRANULAR | (size >>
				     (PAGE_GRAN_SHIFT + GRANULARITY_SHIFT));
  } else {
	segdp->limit_low = size;
	segdp->granularity = size >> GRANULARITY_SHIFT;
  }
  // 默认大小为 32 位
  segdp->granularity |= DEFAULT;	/* means BIG for data seg */
}

/*===========================================================================*
 *				seg2phys				     *
 *===========================================================================*/
PUBLIC phys_bytes seg2phys(seg)
U16_t seg;
{
/* Return the base address of a segment, with seg being either a 8086 segment
 * register, or a 286/386 segment selector.
 */
/*
 * 返回一个段 的段基址, 参数 seg 可以是一个8086 的段寄存器, 也可以是 286/386 
 * 的段选择符.
 */
  phys_bytes base;
  struct segdesc_s *segdp;

  if (! machine.protected) { // 实模式
	base = hclick_to_physb(seg);
  } else {
	segdp = &gdt[seg >> 3];
	base =    ((u32_t) segdp->base_low << 0)
		| ((u32_t) segdp->base_middle << 16)
		| ((u32_t) segdp->base_high << 24);
  }
  return base;
}

/*===========================================================================*
 *				phys2seg				     *
 *===========================================================================*/
// 根据物理地址计算出一个段选择符与段内偏移, 这两个值组合在一起
// 能够映射到该物理地址.
PUBLIC void phys2seg(seg, off, phys)
u16_t *seg;
vir_bytes *off;
phys_bytes phys;
{
/* Return a segment selector and offset that can be used to reach a physical
 * address, for use by a driver doing memory I/O in the A0000 - DFFFF range.
 */
/*
 * 返回一个段选择符和一个偏移量, 利用这两者可以到达一个给定的物理地址.
 * 当驱动程序操作内存 I/O 时会用到. 
 */
  *seg = FLAT_DS_SELECTOR;
  *off = phys;
}

/*===========================================================================*
 *				int_gate				     *
 *===========================================================================*/
// 构造一个中断门描述符, 在 IDT
PRIVATE void int_gate(vec_nr, offset, dpl_type)
unsigned vec_nr;
vir_bytes offset;
unsigned dpl_type;
{
/* Build descriptor for an interrupt gate. */
  register struct gatedesc_s *idp;

  idp = &idt[vec_nr];
  idp->offset_low = offset;
  idp->selector = CS_SELECTOR;
  idp->p_dpl_type = dpl_type;
  idp->offset_high = offset >> OFFSET_HIGH_SHIFT;
}

/*===========================================================================*
 *				enable_iop				     * 
 *===========================================================================*/
PUBLIC void enable_iop(pp)
struct proc *pp;
{
/* Allow a user process to use I/O instructions.  Change the I/O Permission
 * Level bits in the psw. These specify least-privileged Current Permission
 * Level allowed to execute I/O instructions. Users and servers have CPL 3. 
 * You can't have less privilege than that. Kernel has CPL 0, tasks CPL 1.
 */
/*
 * 允许一个用户进程使用 I/O 指针. 在 PSW 中改变 I/O 特权级. 这指定了最低
 * 的, 允许执行 I/O 指令的 当前许可级. 用户进程和服务器进程 的 CPL 是 3.
 * 你的特权级不能比这高. 内核的 CPL 是 0, 内核任务 是 1.
 */
  pp->p_reg.psw |= 0x3000;
}

/*===========================================================================*
 *				alloc_segments				     *
 *===========================================================================*/
// 设置进程的段描述, 寄存器等
PUBLIC void alloc_segments(rp)
register struct proc *rp;
{
/* This is called at system initialization from main() and by do_newmap(). 
 * The code has a separate function because of all hardware-dependencies.
 * Note that IDLE is part of the kernel and gets TASK_PRIVILEGE here.
 */
/*
 * 当系统从 main() 开始初始化, 或者调用  do_newmap() 时, 该函数被调用.
 * 代码包含几个单独的函数, 这是由于硬件依赖导致的. 注意到, IDLE 是内核
 * 的一部分, 在这里它能得到 TASK_PRIVILEGE.
 */
  phys_bytes code_bytes;
  phys_bytes data_bytes;
  int privilege;

  // 如果机器处在保护模式下
  if (machine.protected) {
      // 计算堆栈段的基地址, 内存高端, 得到的是数据段的长度 ???
      data_bytes = (phys_bytes) (rp->p_memmap[S].mem_vir + 
          rp->p_memmap[S].mem_len) << CLICK_SHIFT;
      // 计算代码段的长度, 如果进程的代码段长度为0, 则令 code_bytes 等于
      // 堆栈段的基地址, 在内存高端.
      if (rp->p_memmap[T].mem_len == 0)
          code_bytes = data_bytes;	/* common I&D, poor protect */
      else
          code_bytes = (phys_bytes) rp->p_memmap[T].mem_len << CLICK_SHIFT;
      // 获取进程的特权级
      privilege = (iskernelp(rp)) ? TASK_PRIVILEGE : USER_PRIVILEGE;
      // 初始化代码段描述符
      init_codeseg(&rp->p_ldt[CS_LDT_INDEX],
          (phys_bytes) rp->p_memmap[T].mem_phys << CLICK_SHIFT,
          code_bytes, privilege);
      // 初始化数据段描述符
      init_dataseg(&rp->p_ldt[DS_LDT_INDEX],
          (phys_bytes) rp->p_memmap[D].mem_phys << CLICK_SHIFT,
          data_bytes, privilege);
      // 设置段选择符
      rp->p_reg.cs = (CS_LDT_INDEX * DESC_SIZE) | TI | privilege;
      rp->p_reg.gs =
      rp->p_reg.fs =
      rp->p_reg.ss =
      rp->p_reg.es =
      rp->p_reg.ds = (DS_LDT_INDEX*DESC_SIZE) | TI | privilege;
  } else {
	// 实模式
      rp->p_reg.cs = click_to_hclick(rp->p_memmap[T].mem_phys);
      rp->p_reg.ss =
      rp->p_reg.es =
      rp->p_reg.ds = click_to_hclick(rp->p_memmap[D].mem_phys);
  }
}

