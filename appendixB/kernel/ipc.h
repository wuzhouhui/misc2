#ifndef IPC_H
#define IPC_H

/* This header file defines constants for MINIX inter-process communication.
 * These definitions are used in the file proc.c.
 */
#include <minix/com.h>

/* Masks and flags for system calls. */
#define SYSCALL_FUNC	0x0F	/* mask for system call function */
#define SYSCALL_FLAGS   0xF0    /* mask for system call flags */
#define NON_BLOCKING    0x10	/* prevent blocking, return error */

/* System call numbers that are passed when trapping to the kernel. The 
 * numbers are carefully defined so that it can easily be seen (based on 
 * the bits that are on) which checks should be done in sys_call().
 */
/*
 * 当陷入内核时被递送的系统调用号. 这些号码被认真地定义过, 于是在 sys_call()
 * 中可以很容易判断出应该作哪些检查
 */
// 阻塞发送
#define SEND		   1	/* 0 0 0 1 : blocking send */
// 阻塞接收
#define RECEIVE		   2	/* 0 0 1 0 : blocking receive */
// SENDREC = SEND | RECEIVE
#define SENDREC	 	   3  	/* 0 0 1 1 : SEND + RECEIVE */
// 非阻塞通知
#define NOTIFY		   4	/* 0 1 0 0 : nonblocking notify */
// 回射一个消息
#define ECHO		   8	/* 1 0 0 0 : echo a message */

/* The following bit masks determine what checks that should be done. */
// 检查消息缓冲区
#define CHECK_PTR       0x0B	/* 1 0 1 1 : validate message buffer */
// 检查消息的目标进程.
#define CHECK_DST       0x05	/* 0 1 0 1 : validate message destination */
#define CHECK_SRC       0x02	/* 0 0 1 0 : validate message source */

#endif /* IPC_H */
