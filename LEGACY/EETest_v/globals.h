/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#ifndef	__praxis_globals_h__
#define	__praxis_globals_h__

#ifdef	__SMURPH__

#define	THREADNAME(a)	a ## _test

#include "node.h"
#include "stdattr.h"

// Attribute conversion

// None

// ====================

#else	/* PICOS */

#include "sysio.h"
#include "ser.h"
#include "serf.h"
#include "form.h"

heapmem {10, 90};

#endif	/* SMURPH or PICOS */

#endif
