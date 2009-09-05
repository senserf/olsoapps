#ifndef __vuee_snd_h__
#define	__vuee_snd_h__

#ifdef	__vuee_rcv_h__
#error	"vuee_snd.h and vuee_rcv.h must not be included together!!!"
#endif

#ifdef __SMURPH__

#define	THREADNAME(a)	a ## _snd

#include "board.h"

station NodeSnd : PicOSNode {

// ============================================================================

#define	__dcx_def__
#include "app_snd_data.h"
#undef	__dcx_def__

// ============================================================================

	void appStart ();
	void reset () { PicOSNode::reset (); };
	void init ();
};

#define	_daprx(a)	_dan (NodeSnd, a)

#include "stdattr.h"

strandhdr (sender, NodeSnd) {

	char *data;

	states { SN_SEND };

	perform;

	void setup (char *bf) { data = bf; };
};

threadhdr (root, NodeSnd) {

	states { RS_INIT, RS_RCMD_M, RS_RCMD, RS_RCMD_E, RS_XMIT };

	perform;
};

#else	/* PICOS */

#include "sysio.h"

#define	SN_SEND		0

#define	RS_INIT		0
#define	RS_RCMD_M	1
#define	RS_RCMD		2
#define	RS_RCMD_E	3
#define	RS_XMIT		4

procname (sender);
procname (root);

#endif	/* SMURPH or PICOS */

#define	__dcx_dcl__
#include "app_snd_data.h"
#undef	__dcx_dcl__

#endif
