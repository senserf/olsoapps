#ifndef	__oss_h__
#define	__oss_h__

//+++ "oss.cc"

#include "abb.h"

fsm oss_handler;

//
// Returned values:
//
//	If OSS_buf is not NULL, it contains a return message; the value
//	returned by the function is then ignored; otherwise, the value is
//	sent back in an OSS_CODE_STA message
//
word oss_request (word);
void oss_clear ();

extern byte *OSS_buf;
extern word OSS_bufl;

// Word access
#define	OSS_bufw	((address)OSS_buf)
#define	OSS_bufwl	(OSS_bufl >> 1)

// This is in bytes and includes statid (which in fact belongs to payload in
// this case), but excludes checksum (which doesn't). XRS needs four bytes for
// its own header, which yields the effective length of OSS_MAX_PLEN-4.
#define	OSS_MAX_PLEN		96
// Effective (buffer) length
#define	OSS_MAX_BLEN		(OSS_MAX_PLEN-4)

// Reserved command codes
#define	OSS_CODE_RSA		0xFFF	// Reset (resync) 0
#define	OSS_CODE_RSB		0xFFE	// Reset (resync) 1
#define	OSS_CODE_MEMDUMP	0xF7E	// Memory dump
#define	OSS_CODE_MEMSET		0xF7F	// Memory set

// Standard status codes
#define	OSS_STAT_OK		0x0	// Status OK
#define	OSS_STAT_UNIMPL		0x1	// Command unimplemented
#define	OSS_STAT_ERR		0x2	// Generic error
#define	OSS_STAT_LATER		0x3	// Busy

#define	oss_cmd(b)		(*((word*)(b)) >> 4)
#define	oss_sta(b)		(*((word*)(b)) & 15)
#define	oss_hdr(c,s)		(((c) << 4) | (s))

#ifndef __SMURPH__
// Memory dump compiled in
#define	OSS_MEMORY_ACCESS
#endif

#endif
