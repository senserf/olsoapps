#ifndef	__praxis_globals_h__
#define	__praxis_globals_h__

#ifdef	__SMURPH__

#include "node.h"
#include "stdattr.h"

// Attribute conversion

#define	sfd		_dac (Node, sfd)
#define	last_snt	_dac (Node, last_snt)
#define	last_run	_dac (Node, last_run)
#define my_id		_dac (Node, my_id)
#define rf_start	_dac (Node, rf_start)
#define max_rss		_dac (Node, max_rss)

// ====================

#else	/* PICOS */

#include "sysio.h"
#include "tcvphys.h"
#include "ser.h"
#include "serf.h"
#include "form.h"
#include "phys_cc1100.h"
#include "plug_null.h"

lword	my_id;
int	sfd;
word	last_snt;
word	last_run;
word	rf_start;
word	max_rss;

heapmem {10, 90};

#endif	/* SMURPH or PICOS */

#endif
