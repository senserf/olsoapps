#ifndef	__praxis_globals_h__
#define	__praxis_globals_h__

#ifdef	__SMURPH__

#define	THREADNAME(a)	a ## _test

#include "node.h"
#include "stdattr.h"

// Attribute conversion

#define	sfd		_dac (Node, sfd)
#define	last_snt	_dac (Node, last_snt)
#define	last_rcv	_dac (Node, last_rcv)
#define	last_ack	_dac (Node, last_ack)
#define	XMTon		_dac (Node, XMTon)
#define	RCVon		_dac (Node, RCVon)
#define	rkillflag	_dac (Node, rkillflag)
#define	tkillflag	_dac (Node, tkillflag)

#define	rcv_start	_dac (Node, rcv_start)
#define	rcv_stop	_dac (Node, rcv_stop)
#define	snd_start	_dac (Node, snd_start)
#define	snd_stop	_dac (Node, snd_stop)

// ====================

#else	/* PICOS */

#include "sysio.h"
#include "tcvphys.h"
#include "ser.h"
#include "serf.h"
#include "form.h"
#include "phys_cc1100.h"
#include "plug_null.h"
#include "pinopts.h"

int	sfd;
long	last_snt;
long	last_rcv;
long	last_ack;
bool	XMTon;
bool 	RCVon;
bool	rkillflag;
bool	tkillflag;

heapmem {10, 90};

#endif	/* SMURPH or PICOS */

#endif
