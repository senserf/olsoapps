/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#ifndef __tarp_virt_h
#define __tarp_virt_h

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
