#ifndef __ossi_h__
#define __ossi_h__
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2012                           */
/* All rights reserved.                                                 */
/* ==================================================================== */

// the header becomes invisible if it moves to OSSI(subdir) dupa
#ifdef BOARD_CHRONOS
//#include "ossi_chro.h"
//+++ ossi_chro.cc

#else

//#include "ossi_ser.h"
//+++ ossi_ser.cc
#endif

#endif
