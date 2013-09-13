#ifndef __chro_h__
#define __chro_h__
/* ==================================================================== */
/* Copyright (C) Olsonet Communications 2013                           */
/* All rights reserved.                                                 */
/* ==================================================================== */
//+++ chro.cc
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
void buzzer_init ();
void buzzer_on ();
void buzzer_off ();
#else
#include "ez430_lcd.h"
#include "pins.h"
#endif
#endif
