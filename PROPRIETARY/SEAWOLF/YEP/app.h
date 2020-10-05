/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#ifndef	__app_h__
#define	__app_h__

#include "sysio.h"
#include "pins.h"
#include "oep.h"
#include "oep_ee.h"
#include "lcdg_images.h"
#include "lcdg_dispman.h"
#include "plug_null.h"
#include "phys_uart.h"
#include "ab.h"
#include "sealists.h"
#include "msg.h"
#include "net.h"
#include "sealib.h"

#define	MAXLINES	32
#define	MAXOBJECTS	32

#define	NPVALUES	12
#define	LABSIZE		(LCDG_IM_LABLEN+2)

#endif
