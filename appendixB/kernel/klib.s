#
! Chooses between the 8086 and 386 versions of the low level kernel code.
! 机器状态来选择低层内核代码应该是 8086 版还是 386 版.

#include <minix/config.h>
#if _WORD_SIZE == 2	// 如果字长是2个字节, 则选择 8086, 否则 80386.
#include "klib88.s"
#else
#include "klib386.s"
#endif
