#ifndef __vuee_rcv_h__
#define	__vuee_rcv_h__

#ifdef	__vuee_snd_h__
#error	"vuee_snd.h and vuee_rcv.h must not be included together!!!"
#endif

#ifdef __SMURPH__

#define	THREADNAME(a)	a ## _rcv

#include "board.h"

station NodeRcv : PicOSNode {

// ============================================================================

#define	__dcx_def__
#include "app_rcv_data.h"
#undef	__dcx_def__

// ============================================================================

	void appStart ();
	// void setup (data_no_t*);
	void reset () { PicOSNode::reset (); };
	void init ();
};

#define	_daprx(a)	_dan (NodeRcv, a)

#include "stdattr.h"

threadhdr (receiver, NodeRcv) {

	states { RC_TRY, RC_SHOW };

	perform;

};

threadhdr (root, NodeRcv) {

	states { RS_INIT };

	perform;
};

#else	/* PICOS */

#include "sysio.h"

#define	RC_TRY		0
#define	RC_SHOW		1

#define	RS_INIT		0

procname (receiver);
procname (root);

#endif	/* SMURPH or PICOS */

#define	__dcx_dcl__
#include "app_rcv_data.h"
#undef	__dcx_dcl__

#endif
