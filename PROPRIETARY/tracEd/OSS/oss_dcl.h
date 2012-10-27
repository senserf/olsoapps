/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2012                           */
/* All rights reserved.                                                 */
/* ==================================================================== */
#ifndef __oss_dcl_h__
#define	__oss_dcl_h__

#include "app.h"
#include "tarp.h"
#include "msg.h"

//+++ oss.cc
void rel_cmd (cmd_t * cmd);
sint req_in (req_t * req);

// to fill in by ossi
fsm ossi_init, ossi_in, ossi_out (char *);

void ossi_stats_out (char * b);
void ossi_trace_out (char * buf, word rssi);
void ossi_help_out ();
void ossi_beac_out ();

#endif
