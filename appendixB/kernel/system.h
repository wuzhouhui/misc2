/* Function prototypes for the system library.  
 * The implementation is contained in src/kernel/system/.  
 *
 * The system library allows access to system services by doing a kernel call.
 * Kernel calls are transformed into request messages to the SYS task that is 
 * responsible for handling the call. By convention, sys_call() is transformed 
 * into a message with type SYS_CALL that is handled in a function do_call(). 
 */ 

#ifndef SYSTEM_H
#define SYSTEM_H

/* Common includes for the system library. */
#include "kernel.h"
#include "proto.h"
#include "proc.h"

/* Default handler for unused kernel calls. */
_PROTOTYPE( int do_unused, (message *m_ptr) );
_PROTOTYPE( int do_exec, (message *m_ptr) );		
_PROTOTYPE( int do_fork, (message *m_ptr) );
_PROTOTYPE( int do_newmap, (message *m_ptr) );
_PROTOTYPE( int do_exit, (message *m_ptr) );
_PROTOTYPE( int do_trace, (message *m_ptr) );	
_PROTOTYPE( int do_nice, (message *m_ptr) );
_PROTOTYPE( int do_copy, (message *m_ptr) );	
#define do_vircopy 	do_copy
#define do_physcopy 	do_copy
_PROTOTYPE( int do_vcopy, (message *m_ptr) );		
#define do_virvcopy 	do_vcopy
#define do_physvcopy 	do_vcopy
_PROTOTYPE( int do_umap, (message *m_ptr) );
_PROTOTYPE( int do_memset, (message *m_ptr) );
_PROTOTYPE( int do_abort, (message *m_ptr) );
_PROTOTYPE( int do_getinfo, (message *m_ptr) );
_PROTOTYPE( int do_privctl, (message *m_ptr) );	
_PROTOTYPE( int do_segctl, (message *m_ptr) );
_PROTOTYPE( int do_irqctl, (message *m_ptr) );
_PROTOTYPE( int do_devio, (message *m_ptr) );
_PROTOTYPE( int do_vdevio, (message *m_ptr) );
_PROTOTYPE( int do_int86, (message *m_ptr) );
_PROTOTYPE( int do_sdevio, (message *m_ptr) );
_PROTOTYPE( int do_kill, (message *m_ptr) );
_PROTOTYPE( int do_getksig, (message *m_ptr) );
_PROTOTYPE( int do_endksig, (message *m_ptr) );
_PROTOTYPE( int do_sigsend, (message *m_ptr) );
_PROTOTYPE( int do_sigreturn, (message *m_ptr) );
_PROTOTYPE( int do_times, (message *m_ptr) );		
_PROTOTYPE( int do_setalarm, (message *m_ptr) );	

#endif	/* SYSTEM_H */



