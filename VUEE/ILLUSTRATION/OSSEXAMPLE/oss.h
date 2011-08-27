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

// Two special message codes
#define	OSS_CODE_MTN		0xFF	// Special
#define	OSS_CODE_STA		0xFE	// Status

// Special requests
#define	OSS_CODE_MTN_RSA	0x00	// Reset A
#define	OSS_CODE_MTN_RSB	0x01	// Reset B
#define	OSS_CODE_MTN_MD		0x7E	// Memory dump
#define	OSS_CODE_MTN_MS		0x7F	// RAM set

// Some standard STA codes
#define	OSS_CODE_STA_OK		0x00	// OK (simple command ACK)
#define	OSS_CODE_STA_NOP	0x01	// Null action (command unimplemented?)
#define	OSS_CODE_STA_ERR	0x02	// Generic error
#define	OSS_CODE_STA_LATER	0x03	// Try again later

#ifndef __SMURPH__
// Memory dump compiled in
#define	OSS_MEMORY_ACCESS
#endif

#endif
