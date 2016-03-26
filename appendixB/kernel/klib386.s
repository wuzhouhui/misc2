# 
! sections

.sect .text; .sect .rom; .sect .data; .sect .bss

#include <minix/config.h>
#include <minix/const.h>
#include "const.h"
#include "sconst.h"
#include "protect.h"

! This file contains a number of assembly code utility routines needed by the
! kernel.  They are:
! 该文件包含大量的汇编编写的实用例程, 这些例程是内核所必须的.

.define	_monitor	! exit Minix and return to the monitor
.define	_int86		! let the monitor make an 8086 interrupt call
.define	_cp_mess	! copies messages from source to destination
.define	_exit		! dummy for library routines
.define	__exit		! dummy for library routines
.define	___exit		! dummy for library routines
.define	___main		! dummy for GCC
.define	_phys_insw	! transfer data from (disk controller) port to memory
.define	_phys_insb	! likewise byte by byte
.define	_phys_outsw	! transfer data from memory to (disk controller) port
.define	_phys_outsb	! likewise byte by byte
.define	_enable_irq	! enable an irq at the 8259 controller
.define	_disable_irq	! disable an irq
.define	_phys_copy	! copy data from anywhere to anywhere in memory
.define	_phys_memset	! write pattern anywhere in memory
.define	_mem_rdw	! copy one word from [segment:offset]
.define	_reset		! reset the system
.define	_idle_task	! task executed when there is no work
.define	_level0		! call a function at level 0
.define	_read_tsc	! read the cycle counter (Pentium and up)
.define	_read_cpu_flags	! read the cpu flags

! The routines only guarantee to preserve the registers the C compiler
! expects to be preserved (ebx, esi, edi, ebp, esp, segment registers, and
! direction bit in the flags).

.sect .text
!*===========================================================================*
!*				monitor					     *
!*===========================================================================*
! PUBLIC void monitor();
! Return to the monitor.

_monitor:
	// 切换到监视器的堆栈
	mov	esp, (_mon_sp)		! restore monitor stack pointer
    o16 mov	dx, SS_SELECTOR		! monitor data segment
	mov	ds, dx
	mov	es, dx
	mov	fs, dx
	mov	gs, dx
	mov	ss, dx
	pop	edi
	pop	esi
	pop	ebp
    o16 retf				! return to the monitor


!*===========================================================================*
!*				int86					     *
!*===========================================================================*
! PUBLIC void int86();
! ???
_int86:
	cmpb	(_mon_return), 0	! is the monitor there?
	jnz	0f
	movb	ah, 0x01		! an int 13 error seems appropriate
	movb	(_reg86+ 0), ah		! reg86.w.f = 1 (set carry flag)
	movb	(_reg86+13), ah		! reg86.b.ah = 0x01 = "invalid command"
	ret
0:	push	ebp			! save C registers
	push	esi
	push	edi
	push	ebx
	pushf				! save flags
	cli				! no interruptions

	inb	INT2_CTLMASK
	movb	ah, al
	inb	INT_CTLMASK
	push	eax			! save interrupt masks
	mov	eax, (_irq_use)		! map of in-use IRQ's
	and	eax, ~[1<<CLOCK_IRQ]	! keep the clock ticking
	outb	INT_CTLMASK		! enable all unused IRQ's and vv.
	movb	al, ah
	outb	INT2_CTLMASK

	mov	eax, SS_SELECTOR	! monitor data segment
	mov	ss, ax
	xchg	esp, (_mon_sp)		! switch stacks
	push	(_reg86+36)		! parameters used in INT call
	push	(_reg86+32)
	push	(_reg86+28)
	push	(_reg86+24)
	push	(_reg86+20)
	push	(_reg86+16)
	push	(_reg86+12)
	push	(_reg86+ 8)
	push	(_reg86+ 4)
	push	(_reg86+ 0)
	mov	ds, ax			! remaining data selectors
	mov	es, ax
	mov	fs, ax
	mov	gs, ax
	push	cs
	push	return			! kernel return address and selector
    o16	jmpf	20+2*4+10*4+2*4(esp)	! make the call
return:
	pop	(_reg86+ 0)
	pop	(_reg86+ 4)
	pop	(_reg86+ 8)
	pop	(_reg86+12)
	pop	(_reg86+16)
	pop	(_reg86+20)
	pop	(_reg86+24)
	pop	(_reg86+28)
	pop	(_reg86+32)
	pop	(_reg86+36)
	lgdt	(_gdt+GDT_SELECTOR)	! reload global descriptor table
	jmpf	CS_SELECTOR:csinit	! restore everything
csinit:	mov	eax, DS_SELECTOR
	mov	ds, ax
	mov	es, ax
	mov	fs, ax
	mov	gs, ax
	mov	ss, ax
	xchg	esp, (_mon_sp)		! unswitch stacks
	lidt	(_gdt+IDT_SELECTOR)	! reload interrupt descriptor table
	andb	(_gdt+TSS_SELECTOR+DESC_ACCESS), ~0x02  ! clear TSS busy bit
	mov	eax, TSS_SELECTOR
	ltr	ax			! set TSS register

	pop	eax
	outb	INT_CTLMASK		! restore interrupt masks
	movb	al, ah
	outb	INT2_CTLMASK

	add	(_lost_ticks), ecx	! record lost clock ticks

	popf				! restore flags
	pop	ebx			! restore C registers
	pop	edi
	pop	esi
	pop	ebp
	ret


!*===========================================================================*
!*				cp_mess					     *
!*===========================================================================*
! PUBLIC void cp_mess(int src, phys_clicks src_clicks, vir_bytes src_offset,
!		      phys_clicks dst_clicks, vir_bytes dst_offset);
! This routine makes a fast copy of a message from anywhere in the address
! space to anywhere else.  It also copies the source address provided as a
! parameter to the call into the first word of the destination message.
!
! Note that the message size, "Msize" is in DWORDS (not bytes) and must be set
! correctly.  Changing the definition of message in the type file and not
! changing it here will lead to total disaster.

! 该函数从地址空间的任意一个地址, 快速复制消息到另一个地址. 同时将源地址
! (由输入参数提供) 复制到目的消息的第一个字中.
!
! 注意消息的大小 "Msize" 以 DWORDS 为单位, 必须设置正确. 如果在类型文件中
! 修改了消息的定义, 却没有对该函数作相应的修改, 这将导致混乱.

!	+---------------+
!	| dst_offset	|
!	+---------------+
!	| dst_clicks	|
!	+---------------+
!	| src_offset	|
!	+---------------+
!	| src_clicks	|
!	+---------------+
!	| src		| <- CM_ARGS + esp
!	+---------------+
!	| eip		|
!	+---------------+
!	| esi		|
!	+---------------+
!	| edi		|
!	+---------------+
!	| ds		|
!	+---------------+
!	| es		|<- esp
!	+---------------+

CM_ARGS	=	4 + 4 + 4 + 4 + 4	! 4 + 4 + 4 + 4 + 4
!		es  ds edi esi eip	proc scl sof dcl dof

	.align	16
! 将消息从源进程复制到目的进程, 消息的第一个双字是发送进程
! 的进程号, 包含在消息的大小在, 因此在复制完进程号后, 
! ds:esi += 4, 可见, 源消息的第一个双字没有复制.
_cp_mess:
	cld
	push	esi
	push	edi
	push	ds
	push	es

	! 重新设置段选择符
	mov	eax, FLAT_DS_SELECTOR
	mov	ds, ax
	mov	es, ax

	! esi = 源地址, edi = 目的地址, esp 先递减, 再压数据
	mov	esi, CM_ARGS+4(esp)		! src clicks
	shl	esi, CLICK_SHIFT		! clicks << 1024
	add	esi, CM_ARGS+4+4(esp)		! src offset
	mov	edi, CM_ARGS+4+4+4(esp)		! dst clicks
	shl	edi, CLICK_SHIFT
	add	edi, CM_ARGS+4+4+4+4(esp)	! dst offset

	mov	eax, CM_ARGS(esp)	! process number of sender
	! es:edi = eax
	stos				! copy number of sender to dest message
	! after stos, edi += 2
	add	esi, 4			! do not copy first word
	mov	ecx, Msize - 1		! remember, first word does not count
	rep
	movs				! copy the message

	pop	es
	pop	ds
	pop	edi
	pop	esi
	ret				! that is all folks!


!*===========================================================================*
!*				exit					     *
!*===========================================================================*
! PUBLIC void exit();
! Some library routines use exit, so provide a dummy version.
! Actual calls to exit cannot occur in the kernel.
! GNU CC likes to call ___main from main() for nonobvious reasons.

_exit:
__exit:
___exit:
	sti
	jmp	___exit

___main:
	ret


!*===========================================================================*
!*				phys_insw				     *
!*===========================================================================*
! PUBLIC void phys_insw(Port_t port, phys_bytes buf, size_t count);
! Input an array from an I/O port.  Absolute address version of insw().

_phys_insw:
	push	ebp
	mov	ebp, esp
	cld
	push	edi
	push	es
	mov	ecx, FLAT_DS_SELECTOR
	mov	es, cx
	mov	edx, 8(ebp)		! port to read from
	mov	edi, 12(ebp)		! destination addr
	mov	ecx, 16(ebp)		! byte count
	shr	ecx, 1			! word count
rep o16	ins				! input many words
	pop	es
	pop	edi
	pop	ebp
	ret


!*===========================================================================*
!*				phys_insb				     *
!*===========================================================================*
! PUBLIC void phys_insb(Port_t port, phys_bytes buf, size_t count);
! Input an array from an I/O port.  Absolute address version of insb().
!	+---------------+
!	  count 
!	  buf 
!	  port 
!	  eip 
!	  ebp	<-- ebp 
!	  edi 
!	  es 

_phys_insb:
	push	ebp
	mov	ebp, esp
	cld
	push	edi
	push	es
	mov	ecx, FLAT_DS_SELECTOR
	mov	es, cx
	mov	edx, 8(ebp)		! port to read from
	mov	edi, 12(ebp)		! destination addr
	mov	ecx, 16(ebp)		! byte count
!	shr	ecx, 1			! word count
   rep	insb				! input many bytes
	pop	es
	pop	edi
	pop	ebp
	ret


!*===========================================================================*
!*				phys_outsw				     *
!*===========================================================================*
! PUBLIC void phys_outsw(Port_t port, phys_bytes buf, size_t count);
! Output an array to an I/O port.  Absolute address version of outsw().

	.align	16
_phys_outsw:
	push	ebp
	mov	ebp, esp
	cld
	push	esi
	push	ds
	mov	ecx, FLAT_DS_SELECTOR
	mov	ds, cx
	mov	edx, 8(ebp)		! port to write to
	mov	esi, 12(ebp)		! source addr
	mov	ecx, 16(ebp)		! byte count
	shr	ecx, 1			! word count
rep o16	outs				! output many words
	pop	ds
	pop	esi
	pop	ebp
	ret


!*===========================================================================*
!*				phys_outsb				     *
!*===========================================================================*
! PUBLIC void phys_outsb(Port_t port, phys_bytes buf, size_t count);
! Output an array to an I/O port.  Absolute address version of outsb().

	.align	16
_phys_outsb:
	push	ebp
	mov	ebp, esp
	cld
	push	esi
	push	ds
	mov	ecx, FLAT_DS_SELECTOR
	mov	ds, cx
	mov	edx, 8(ebp)		! port to write to
	mov	esi, 12(ebp)		! source addr
	mov	ecx, 16(ebp)		! byte count
   rep	outsb				! output many bytes
	pop	ds
	pop	esi
	pop	ebp
	ret


!*==========================================================================*
!*				enable_irq				    *
!*==========================================================================*/
! PUBLIC void enable_irq(irq_hook_t *hook)
! Enable an interrupt request line by clearing an 8259 bit.
! 通过清除一个 8259 位, 来开启一个中断请求线.
! Equivalent C code for hook->irq < 8:
!   if ((irq_actids[hook->irq] &= ~hook->id) == 0)
!	outb(INT_CTLMASK, inb(INT_CTLMASK) & ~(1 << irq));
!!!!!!!!!!!!!!!!!!!!!!!!!!!
!	hook 
!	eip 
!	ebp	<- ebp 
!	flags	<- esp 

	.align	16
_enable_irq:
	push	ebp
	mov	ebp, esp
	pushf
	cli
	mov	eax, 8(ebp)		! hook		eax = hook;
	mov	ecx, 8(eax)		! irq		exc = hook->irq;
	mov	eax, 12(eax)		! id bit	eax = hook->id;
	not	eax
	and	_irq_actids(ecx*4), eax	! clear this id bit
	! 如果 _irq_actids 在清除了 hook->id 对应的位之后, 还有不为
	! 0 的位, 则跳至 en_done
	jnz	en_done			! still masked by other handlers?
	movb	ah, ~1
	rolb	ah, cl			! ah = ~(1 << (irq % 8))
	mov	edx, INT_CTLMASK	! enable irq < 8 at the master 8259
	cmpb	cl, 8
	jb	0f
	mov	edx, INT2_CTLMASK	! enable irq >= 8 at the slave 8259
0:	inb	dx
	andb	al, ah
	outb	dx			! clear bit at the 8259
en_done:popf
	leave
	ret


!*==========================================================================*
!*				disable_irq				    *
!*==========================================================================*/
! PUBLIC int disable_irq(irq_hook_t *hook)
! Disable an interrupt request line by setting an 8259 bit.
! Equivalent C code for irq < 8:
!   irq_actids[hook->irq] |= hook->id;
!   outb(INT_CTLMASK, inb(INT_CTLMASK) | (1 << irq));
! Returns true iff the interrupt was not already disabled.

	.align	16
_disable_irq:
	push	ebp
	mov	ebp, esp
	pushf
	cli
	mov	eax, 8(ebp)		! hook
	mov	ecx, 8(eax)		! irq
	mov	eax, 12(eax)		! id bit
	or	_irq_actids(ecx*4), eax	! set this id bit
	movb	ah, 1
	rolb	ah, cl			! ah = (1 << (irq % 8))
	mov	edx, INT_CTLMASK	! disable irq < 8 at the master 8259
	cmpb	cl, 8
	jb	0f
	mov	edx, INT2_CTLMASK	! disable irq >= 8 at the slave 8259
0:	inb	dx
	testb	al, ah
	jnz	dis_already		! already disabled?
	orb	al, ah
	outb	dx			! set bit at the 8259
	mov	eax, 1			! disabled by this function
	popf
	leave
	ret
dis_already:
	xor	eax, eax		! already disabled
	popf
	leave
	ret


!*===========================================================================*
!*				phys_copy				     *
!*===========================================================================*
! PUBLIC void phys_copy(phys_bytes source, phys_bytes destination,
!			phys_bytes bytecount);
! Copy a block of physical memory.

!	+---------------+
!	| bytecount	|
!	+---------------+
!	| destination	|
!	+---------------+
!	| source	| <--(esp + PC_ARGS)
!	+---------------+
!	| eip		|
!	+---------------+
!	| esi		|
!	+---------------+
!	| edi		|
!	+---------------+
!	| es		| <--esp
!	+---------------+

PC_ARGS	=	4 + 4 + 4 + 4	! 4 + 4 + 4
!		es edi esi eip	 src dst len

	.align	16
_phys_copy:
	cld
	push	esi
	push	edi
	push	es

	mov	eax, FLAT_DS_SELECTOR
	mov	es, ax

	mov	esi, PC_ARGS(esp)	! ds:esi ==> es:edi
	mov	edi, PC_ARGS+4(esp)
	mov	eax, PC_ARGS+4+4(esp)

	! 如果复制的字节数小于 10, 则跳到 pc_small, 以避免因复制小数据量
	! 而带来的对齐开销.
	cmp	eax, 10			! avoid align overhead for small counts
	jb	pc_small
	! 为提高效率, 程序先对源地址进行4字节对齐, 因此这里先把未对齐的
	! 字节(1~3 bytes) 先复制过去.
	mov	ecx, esi		! align source, hope target is too
	neg	ecx		! 可是为什么要取补呢 ???
	and	ecx, 3			! count for alignment
	sub	eax, ecx
	rep
   eseg	movsb	! 每次复制一个字节
	mov	ecx, eax
	shr	ecx, 2			! count of dwords
	rep
   eseg	movs	! 每次复制一个双字, 最后可以会剩下 1 到 3 个字节, 由
		! pc_small 接着完成, 可问题是 CPU 怎么知道 movs 每次 
		! 是复制一个字节, 字, 还是双字
	and	eax, 3
pc_small:
	xchg	ecx, eax		! remainder
	rep	! repeat following statement(eseg movsb)
   eseg	movsb

	pop	es
	pop	edi
	pop	esi
	ret

!*===========================================================================*
!*				phys_memset				     *
!*===========================================================================*
! PUBLIC void phys_memset(phys_bytes source, unsigned long pattern,
!	phys_bytes bytecount);
! Fill a block of physical memory with pattern.
! 用某一模式填充一个物理内存块.
!	+---------------+
!	| bytecount	|
!	+---------------+
!	| pattern	|
!	+---------------+
!	| source	|
!	+---------------+
!	| eip		|
!	+---------------+
!	| ebp		| <-- ebp
!	+---------------+
!	| esi		|
!	+---------------+
!	| ebx		|
!	+---------------+
!	| ds		| <-- esp
!	+---------------+

	.align	16
_phys_memset:
	push	ebp
	mov	ebp, esp
	push	esi
	push	ebx
	push	ds
	mov	esi, 8(ebp)		! esi = source
	mov	eax, 16(ebp)		! eax = bytecount
	mov	ebx, FLAT_DS_SELECTOR
	mov	ds, bx
	mov	ebx, 12(ebp)		! ebx = pattern
	shr	eax, 2			! eax = bytecount >> 2
fill_start:
   	mov     (esi), ebx
	add	esi, 4
	dec	eax
	jnz	fill_start
	! Any remaining bytes?
	mov	eax, 16(ebp)
	and	eax, 3
remain_fill:
	cmp	eax, 0
	jz	fill_done
	movb	bl, 12(ebp)
   	movb    (esi), bl
	add	esi, 1
	inc	ebp
	dec	eax
	jmp	remain_fill
fill_done:
	pop	ds
	pop	ebx
	pop	esi
	pop	ebp
	ret

!*===========================================================================*
!*				mem_rdw					     *
!*===========================================================================*
! PUBLIC u16_t mem_rdw(U16_t segment, u16_t *offset);
! Load and return word at far pointer segment:offset.

	.align	16
_mem_rdw:
	mov	cx, ds
	mov	ds, 4(esp)		! segment
	mov	eax, 4+4(esp)		! offset
	movzx	eax, (eax)		! word to return
	mov	ds, cx
	ret


!*===========================================================================*
!*				reset					     *
!*===========================================================================*
! PUBLIC void reset();
! Reset the system by loading IDT with offset 0 and interrupting.
! 通过载入偏移量为 0 的 IDT 并产生中断来复位系统.

_reset:
	lidt	(idt_zero)
	int	3		! anything goes, the 386 will not like it
.sect .data
idt_zero:	.data4	0, 0
.sect .text


!*===========================================================================*
!*				idle_task				     *
!*===========================================================================*
_idle_task:
! This task is called when the system has nothing else to do.  The HLT
! instruction puts the processor in a state where it draws minimum power.
! 当系统中没有其他进程可运行时, 运行该任务, HLT 指令会把处理置于
! 最省电的状态.
	push	halt
	call	_level0		! level0(halt)
	pop	eax
	jmp	_idle_task
halt:
	sti
	hlt
	cli
	ret

!*===========================================================================*
!*			      level0					     *
!*===========================================================================*
! PUBLIC void level0(void (*func)(void))
! Call a function at permission level 0.  This allows kernel tasks to do
! things that are only possible at the most privileged CPU level.
! 在特权级 0 的状态下调用一个函数. 这允许内核任务在最高的特权级上做一些
! 事情.
_level0:
	mov	eax, 4(esp)		! eax = func
	mov	(_level0_func), eax	! _level0_func ???
	int	LEVEL0_VECTOR
	ret


!*===========================================================================*
!*			      read_tsc					     *
!*===========================================================================*
! PUBLIC void read_tsc(unsigned long *high, unsigned long *low);
! Read the cycle counter of the CPU. Pentium and up. 
! 读取 CPU 的周期计数器, 奔腾以上支持.
!	+---------------+
!	| long *low	|
!	+---------------+
!	| long *high	|
!	+---------------+
!	| eip		|
!	+---------------+
!	| ebp		| <-- esp
!	+---------------+
.align 16
_read_tsc:
! RDTSC 指令的机器码
.data1 0x0f		! this is the RDTSC instruction 
.data1 0x31		! it places the TSC in EDX:EAX
	push ebp
	mov ebp, 8(esp)
	mov (ebp), edx	 ! *high = edx
	mov ebp, 12(esp)
	mov (ebp), eax   ! *low  = eax
	pop ebp
	ret

!*===========================================================================*
!*			      read_flags					     *
!*===========================================================================*
! PUBLIC unsigned long read_cpu_flags(void);
! Read CPU status flags from C.
.align 16
_read_cpu_flags:
	pushf
	mov eax, (esp)
	popf
	ret

