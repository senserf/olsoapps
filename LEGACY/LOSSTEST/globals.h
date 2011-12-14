#ifndef	__praxis_globals_h__
#define	__praxis_globals_h__

#include "params.h"

#ifdef	__SMURPH__

#define	THREADNAME(a)	a ## _test

#include "node.h"
#include "stdattr.h"

// Attribute conversion

#define	sfd		_dac (Node, sfd)
#define	last_snt	_dac (Node, last_snt)
#define	last_rcv	_dac (Node, last_rcv)
#define	received	_dac (Node, received)
#define	lost		_dac (Node, lost)
#define	packet_length	_dac (Node, packet_length)
#define	send_interval	_dac (Node, send_interval)
#define	NODE_ID		_dac (Node, NODE_ID)
#define	receiving	_dac (Node, receiving)

// ====================

#else	/* PICOS */

#include "sysio.h"
#include "tcvphys.h"
#include "ser.h"
#include "serf.h"
#include "form.h"
#include "phys_cc1100.h"
#include "plug_null.h"

int	sfd;
lword	last_snt;
lword	last_rcv [MAX_PEER_COUNT];
lword	received [MAX_PEER_COUNT];
lword	lost [MAX_PEER_COUNT];
word	packet_length;
word	send_interval;
int	sfd;
Boolean	receiving;

const word	NODE_ID = 0xBACA;;

heapmem {10, 90};

#endif	/* SMURPH or PICOS */

#endif
