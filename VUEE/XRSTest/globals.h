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
#include "plug_null.h"
#include "ab.h"

heapmem {10, 90};

#endif	/* SMURPH or PICOS */

#endif
