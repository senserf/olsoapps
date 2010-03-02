#ifndef __vuee_tag_h__
#define __vuee_tag_h__

#ifdef  __vuee_peg_h__
#error  "vuee_peg.h and vuee_tag.h must not be included together!!!"
#endif

#ifdef	__SMURPH__

#undef THREADNAME
#define THREADNAME(a)   a ## _tag

#include "board.h"
#include "app_tag.h"

station NodeTag : PicOSNode {

// ============================================================================

#define __dcx_def__
#include "app_tag_data.h"
#undef  __dcx_def__

// ============================================================================

        void appStart ();
	void reset () { PicOSNode::reset (); };
        void init ();
};

#define _daprx(a)       _dan (NodeTag, a)

#include "stdattr.h"


threadhdr (rcv, NodeTag) {

	states 	{ RC_TRY, RC_MSG };

	perform;
};

threadhdr (sens, NodeTag) {

	states { SE_INIT, SE_0, SE_1, SE_2, SE_3, SE_4, SE_5,
	 	 SE_DONE };

	perform;

};

threadhdr (rxsw, NodeTag) {

	states 	{ RX_OFF, RX_ON };

	perform;

};

threadhdr (pong, NodeTag) {

	states 	{ PS_INIT, PS_NEXT, PS_SEND, PS_SEND1, PS_HOLD,
       		PS_SENS, PS_COLL };

	perform;
};

threadhdr (cmd_in, NodeTag) {

	states { CS_INIT, CS_IN, CS_WAIT };

	perform;
};

strandhdr (oss_out, NodeTag) {

	char *data;

	states { OO_RETRY };

	void setup (char *d) { data = d; };

	perform;
};

threadhdr (root, NodeTag) {

	states { RS_INIT, RS_INIT1, RS_INIT2, RS_PAUSE, RS_FREE, RS_RCMD,
	       	RS_DOCMD, RS_UIOUT, RS_DUMP };

	perform;
};

#else	/* PICOS */

#include "sysio.h"

int sens (word, address);
int rxsw (word, address);
int pong (word, address);

procname (sens);
#define SE_INIT		0
#define SE_0		10
#define SE_1		20
#define SE_2		30
#define SE_3		40
#define SE_4		50
#define SE_5		60
#define SE_DONE		70

procname (rcv);
#define	RC_TRY		0
#define	RC_MSG		10

procname (rxsw);
#define	RX_OFF		0
#define	RX_ON		10

procname (pong);
#define	PS_INIT		0
#define	PS_NEXT		10
#define	PS_SEND		20
#define	PS_SEND1	30
#define PS_HOLD		40
#define PS_SENS		50
#define PS_COLL		60

procname (cmd_in);

#define	CS_INIT		0
#define	CS_IN		10
#define	CS_WAIT		20

procname (oss_out);
#define OO_RETRY        0

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

#endif

#define __dcx_dcl__
#include "app_tag_data.h"
#undef  __dcx_dcl__

#endif
