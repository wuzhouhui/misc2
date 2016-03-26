/* This file provides basic types and some constants for the 
 * SYS_DEVIO and SYS_VDEVIO system calls, which allow user-level 
 * processes to perform device I/O. 
 *
 * Created: 
 *	Apr 08, 2004 by Jorrit N. Herder
 */

#ifndef _DEVIO_H
#define _DEVIO_H

#include <minix/sys_config.h>     /* needed to include <minix/type.h> */
#include <sys/types.h>        /* u8_t, u16_t, u32_t needed */

typedef u16_t port_t;
typedef U16_t Port_t;

/* We have different granularities of port I/O: 8, 16, 32 bits.
 * Also see <ibm/portio.h>, which has functions for bytes, words,  
 * and longs. Hence, we need different (port,value)-pair types. 
 */
typedef struct { u16_t port;  u8_t value; } pvb_pair_t;
typedef struct { u16_t port; u16_t value; } pvw_pair_t;
typedef struct { u16_t port; u32_t value; } pvl_pair_t;

/* Macro shorthand to set (port,value)-pair. */
#define pv_set(pv, p, v) ((pv).port = (p), (pv).value = (v))
#define pv_ptr_set(pv_ptr, p, v) ((pv_ptr)->port = (p), (pv_ptr)->value = (v))

#endif  /* _DEVIO_H */
