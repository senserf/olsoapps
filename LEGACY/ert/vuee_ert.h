#ifndef __vuee_ert_h__
#define	__vuee_ert_h__

#ifdef	__vuee_dum_h__
#error	"vuee_ert.h and vuee_dum.h must not be included together!!!"
#endif

#ifdef __SMURPH__

#undef THREADNAME
#define	THREADNAME(a)	a ## _ert

#include "board.h"
#include "app_ert.h"

station NodeErt : PicOSNode {

// ============================================================================

#define	__dcx_def__
#include "app_ert_data.h"
#undef	__dcx_def__

// ============================================================================

	void appStart ();
	void reset () { PicOSNode::reset (); };
	void init ();
};

#define	_daprx(a)	_dan (NodeErt, a)

#include "stdattr.h"

threadhdr (root, NodeErt) {

	states { RS_INIT, RS_FREE, RS_RCMD, RS_DOCMD, RS_CMDOUT, RS_RETOUT };

	perform;
};

threadhdr (rcv, NodeErt) {

        states { RS_TRY, RS_MSG };

        perform;
};

#if UART_DRIVER || UART_TCV

threadhdr (cmd_in, NodeErt) {

        states { CS_START, CS_TOUT, CS_IN, CS_READ_START, CS_READ_LEN,
		 CS_READ_BODY, CS_DONE_R, CS_WAIT };

        perform;
};

threadhdr (dat_in, NodeErt) {

        states { DS_START, DS_TOUT, DS_IN, DS_READ_START, DS_READ };

        perform;
};

strandhdr (oss_out, NodeErt) {

	char *data;

        states { OO_RETRY };

        perform;

	void setup (char *b) { data = b; };
};

// if UART_DRIVER || UART_TCV
#endif

strandhdr (app_outserial, NodeErt) {

        char *data;

        states { OM_INIT, OM_WRITE };

        perform;

        void setup (char *b) { data = b; };
};

threadhdr (st_rep, NodeErt) {

        states { SRS_INIT, SRS_DEL, SRS_ITER, SRS_BR, SRS_FIN,
		 ST_REP_BOOT_DELAY };

        perform;
};

threadhdr (dat_rep, NodeErt) {

        states { DRS_INIT, DRS_REP, DRS_FIN };

        perform;
};

threadhdr (io_back, NodeErt) {

        states { IBS_ITER, IBS_HOLD };

        perform;
};

threadhdr (io_rep, NodeErt) {

        states { IRS_INIT, IRS_ITER, IRS_IRUPT, IRS_REP, IRS_FIN };

        perform;
};

threadhdr (cyc_man, NodeErt) {

        states { CYS_INIT, CYS_ACT, CYS_HOLD };

        perform;
};

strandhdr (beacon, NodeErt) {

	char *data;

        states { BS_ITER, BS_ACT };

        perform;

	void setup (char *b) { data = b; };
};

threadhdr (con_man, NodeErt) {

        states { COS_INIT, COS_ITER, COS_ACT };

        perform;
};

#else	/* PICOS */

#include "sysio.h"

procname (root);
#define	RS_INIT		0
#define	RS_FREE		10
#define	RS_RCMD		20
#define	RS_DOCMD	30
#define	RS_CMDOUT	40
#define RS_RETOUT	50

procname (rcv);
#define RS_TRY		0
#define RS_MSG		10

#if UART_DRIVER || UART_TCV

procname (cmd_in);
#define CS_START        0
#define CS_TOUT         10
#define CS_IN           20
#define CS_READ_START   30
#define CS_READ_LEN     40
#define CS_READ_BODY    50
#define CS_DONE_R       60
#define CS_WAIT         70

procname (dat_in);
#define DS_START        0
#define DS_TOUT         10
#define DS_IN           20
#define DS_READ_START   30
#define DS_READ         40

procname (oss_out);
#define OO_RETRY 0

// if UART_DRIVER || UART_TCV
#endif

procname (app_outserial);
#define OM_INIT         0
#define OM_WRITE        10

procname (st_rep);
#define SRS_INIT        0
#define SRS_DEL         10
#define SRS_ITER        20
#define SRS_BR          30
#define SRS_FIN         40
#define ST_REP_BOOT_DELAY 50

procname (dat_rep);
#define DRS_INIT        0
#define DRS_REP         10
#define DRS_FIN         20

procname (io_back);
#define IBS_ITER        0
#define IBS_HOLD        10

procname (io_rep);
#define IRS_INIT        0
#define IRS_ITER        10
#define IRS_IRUPT       20
#define IRS_REP         30
#define IRS_FIN         40

procname (cyc_man);
#define CYS_INIT        0
#define CYS_ACT         10
#define CYS_HOLD        20

procname (beacon);
#define BS_ITER 	0
#define BS_ACT  	10

procname (con_man);
#define COS_INIT 	0
#define COS_ITER 	10
#define COS_ACT  	20

#endif	/* SMURPH or PICOS */

#define	__dcx_dcl__
#include "app_ert_data.h"
#undef	__dcx_dcl__

#endif
