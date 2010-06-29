#ifndef	__vuee_peg_h__
#define	__vuee_peg_h__

#ifdef  __vuee_tag_h__
#error  "vuee_peg.h and vuee_tag.h must not be included together!!!"
#endif

#ifdef	__SMURPH__

#undef THREADNAME
#define THREADNAME(a)   a ## _peg

#include "board.h"
#include "app_peg.h"

station NodePeg : PicOSNode {

// ============================================================================

#define __dcx_def__
#include "app_peg_data.h"
#undef  __dcx_def__

// ============================================================================

	void appStart ();
	void reset () { PicOSNode::reset (); };
	void init ();
};

#define _daprx(a)	_dan (NodePeg, a)

#include "stdattr.h"

threadhdr (rcv, NodePeg) {

	states 	{ RC_TRY, RC_MSG };

	perform;
};

threadhdr (audit, NodePeg) {

	states 	{ AS_START, AS_TAGLOOP, AS_TAGLOOP1, AS_HOLD };

	perform;

};

threadhdr (cmd_in, NodePeg) {

	states { CS_INIT, CS_IN, CS_WAIT };

	perform;
};

threadhdr (mbeacon, NodePeg) {

	states { MB_START, MB_SEND };

	perform;
};

strandhdr (oss_out, NodePeg) {

	char *data;

	states { OO_START, OO_RETRY };

	perform;

	void setup (char *d) { data = d; };

};

threadhdr (root, NodePeg) {

	states { RS_INIT, RS_INIT1, RS_INIT2, RS_PAUSE, RS_FREE, RS_RCMD,
	       	RS_DOCMD, RS_UIOUT, RS_DUMP, RS_INIEE };

	perform;
};

#else	/* PICOS */

#include "sysio.h"

procname (rcv);
#define	RC_TRY		0
#define	RC_MSG		10

procname (audit);
#define	AS_START	0
#define	AS_TAGLOOP	10
#define	AS_TAGLOOP1	20
#define AS_HOLD		30

procname (cmd_in);
#define	CS_INIT		0
#define	CS_IN		10
#define	CS_WAIT		20

procname (mbeacon);
#define MB_START        0
#define MB_SEND		10

procname (oss_out);
#define	OO_START	0
#define OO_RETRY	10

procname (root);
#define RS_INIT		0
#define RS_INIT1	10
#define RS_INIT2	20
#define RS_PAUSE	30
#define RS_FREE		40
#define RS_RCMD		50
#define RS_DOCMD	60
#define RS_UIOUT	70
#define RS_DUMP		80
#define RS_INIEE	90

#endif

#define __dcx_dcl__
#include "app_peg_data.h"
#undef  __dcx_dcl__

#endif
