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
extern beac_t		beac;
extern odr_t		odr;
extern trac_t		trac;
extern disp_t		disp;
extern fim_t		fim_set;

//dupa in tarp.h... why, if uncommented, it crashes picomp
// (they're in tarp.h)
//extern nid_t		local_host, master_host;

char * get_mem (word state, word len);
void  init ();
word  fim_read ();
word  fim_write ();

// FSM announcements are only needed for PicOS. From the viewpoint of VUEE,
// all FSM's are global (within the set of one praxis).
//
fsm mbeacon;

#endif
