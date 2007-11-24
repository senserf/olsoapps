#ifndef __nvm_h
#define __nvm_h
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2006.                   */
/* All rights reserved.                                                 */
/* ==================================================================== */
//+++ "nvm.c"
#include "sysio.h"

#define NVM_PAGE_SIZE	64

// IMPORTANT: always keep them TOGETHER
#define NVM_BOOT_LEN	15
#define NVM_NID		0
#define NVM_LH		(NVM_NID + 1)
#define NVM_MID		(NVM_NID + 2)

// NVM_APP: b0-b2, b3: encr; b4: binder; b5: cmdmode; b6: dat cr; b7 spare
//          b8-b15: tarp_ctrl.param
#define NVM_APP		(NVM_NID + 3)

#define NVM_UART	(NVM_NID + 4)
#define NVM_CYC_CTRL    (NVM_NID + 5)
#define NVM_CYC_SP	(NVM_NID + 6)

#define NVM_IO_CMP	(NVM_NID + 8)
#define NVM_IO_CREG	(NVM_NID + 10)
#define NVM_IO_PINS	(NVM_NID + 12)
// 1 byte free in RSSI_THOLD
#define NVM_RSSI_THOLD	(NVM_NID + 14)

// now, it is on dedicated page 1
#define NVM_IO_STATE	(NVM_PAGE_SIZE)

extern void nvm_read (word pos, address d, word wlen);
extern void nvm_write (word pos, const word * s, word wlen);
extern void nvm_erase();
extern void nvm_io_backup();
extern void app_reset (word lev);
#endif

