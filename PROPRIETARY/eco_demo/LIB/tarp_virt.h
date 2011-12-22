#ifndef __tarp_virt_h
#define __tarp_virt_h
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2011 			*/
/* All rights reserved.							*/
/* ==================================================================== */

// Headers of TARP's virtual functions

#include "tarp.h"

idiosyncratic int tr_offset (headerType*);
idiosyncratic Boolean msg_isBind (msg_t);
idiosyncratic Boolean msg_isTrace (msg_t);
idiosyncratic Boolean msg_isMaster (msg_t);
idiosyncratic Boolean msg_isNew (msg_t);
idiosyncratic Boolean msg_isClear (byte);
idiosyncratic void set_master_chg ();

#endif
