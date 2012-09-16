/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2012                           */
/* All rights reserved.                                                 */
/* ==================================================================== */
#ifndef __app_dcl_h__
#define	__app_dcl_h__

#include "sysio.h"
#include "app.h"
#include "tarp.h"

//+++ utils.cc

extern lword		master_ts;
extern appfl_t		app_flags;
extern word		bat;
//dupa in tarp.h... why, if uncommented, it crashes picomp?
// extern nid_t		local_host, master_host;

char * get_mem (word state, word len);
void  init ();

// FSM announcements are only needed for PicOS. From the viewpoint of VUEE,
// all FSM's are global (within the set of one praxis).
//
fsm mbeacon;

#endif
