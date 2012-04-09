#ifndef __chro_col_h__
#define __chro_col_h__
/* ==================================================================== */
/* Copyright (C) Olsonet Communications 2012                           */
/* All rights reserved.                                                 */
/* ==================================================================== */
//+++ chro_col.cc
#include "sysio.h"
void chro_hi (const char *txt);
void chro_lo (const char *txt);
void chro_nn (word hi, word a);
void chro_xx (word hi, word a);
#ifdef __SMURPH__
void ezlcd_init ();
void ezlcd_on ();
void ezlcd_off ();
void cma3000_on (byte, byte, byte);
void cma3000_off ();
#else
#include "ez430_lcd.h"
#endif
#endif
