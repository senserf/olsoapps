#ifndef __vuee_h__
#define	__vuee_h__

#ifdef __SMURPH__

#define	THREADNAME(a)	a ## _snd

#include "board.h"

station NodeTest : PicOSNode {

// ============================================================================

#define	__dcx_def__
#include "app_data.h"
#undef	__dcx_def__

// ============================================================================

	void appStart ();
	void reset () { PicOSNode::reset (); };
	void init ();
};

#define	_daprx(a)	_dan (NodeTest, a)

#include "stdattr.h"

threadhdr (thread_rfcmd, NodeTest) {

	states { RC_START, RC_SEND };
	perform;
};

threadhdr (thread_rreporter, NodeTest) {

	states { UF_START, UF_FIXED, UF_NEXT, UF_DRIV };
	perform;
};

threadhdr (thread_ureporter, NodeTest) {

	states { RE_START, RE_FIXED, RE_NEXT, RE_DRIV };
	perform;
};

threadhdr (thread_listener, NodeTest) {

	states { LI_WAIT } ;
	perform;
};

threadhdr (thread_sender, NodeTest) {

	states { SN_DELAY, SN_NEXT, SN_NSEN };
	perform;
};

threadhdr (root, NodeTest) {

	states { RS_INIT, RS_BANNER, RS_ON, RS_READ, RS_MSG };
	perform;
};

#else	/* PICOS */

//+++ "hostid.c"

#include "sysio.h"

#define	RC_START	0
#define	RC_SEND		1

#define	UF_START	0
#define UF_FIXED	1
#define	UF_NEXT		2
#define	UF_DRIV		3

#define	RE_START	0
#define RE_FIXED	1
#define	RE_NEXT		2
#define	RE_DRIV		3

#define	LI_WAIT		0

#define	SN_DELAY	0
#define	SN_NEXT		1
#define	SN_NSEN		2

#define	RS_INIT		0
#define	RS_BANNER	1
#define	RS_ON		2
#define	RS_READ		3
#define RS_MSG		4
	
procname (thread_rfcmd);
procname (thread_rreporter);
procname (thread_ureporter);
procname (thread_listener);
procname (thread_sender);
procname (root);

#endif	/* SMURPH or PICOS */

#define	__dcx_dcl__
#include "app_data.h"
#undef	__dcx_dcl__

#endif
