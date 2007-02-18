#ifndef	__praxis_globals_h__
#define	__praxis_globals_h__

#ifdef	__SMURPH__

#define	THREADNAME(a)	a

#include "node.h"
#include "stdattr.h"

// Attribute conversion

#define	sfd		_dac (Node, sfd)
#define	Count		_dac (Node, Count)

#define	show		_dac (Node, show)
#define	plen		_dac (Node, plen)

// ====================

#else	/* PICOS */

#include "sysio.h"
#include "tcvphys.h"
#include "ser.h"
#include "serf.h"
#include "form.h"
#include "phys_dm2200.h"
#include "plug_null.h"

int	sfd;
word	Count;

heapmem {10, 90};

#endif	/* SMURPH or PICOS */

#endif
