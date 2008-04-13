#ifndef	__threadhdrs_peg_h__
#define	__threadhdrs_peg_h__

#ifdef	__SMURPH__

threadhdr (rcv, NodePeg) {

	int	rcv_packet_size;
	char 	*rcv_buf_ptr;
	word 	rcv_rssi;

	states 	{ RC_INIT, RC_TRY, RC_MSG };

	perform;
};

threadhdr (audit, NodePeg) {

	lword lh_time;
	char *aud_buf_ptr;
	word aud_ind;

	states 	{ AS_INIT, AS_START, AS_TAGLOOP, AS_TAGLOOP1, AS_HOLD };

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

	states { OO_RETRY };

	void setup (char *d) { data = d; };

	perform;
};

threadhdr (root, NodePeg) {

	states { RS_INIT, RS_PAUSE, RS_FREE, RS_RCMD, RS_DOCMD, RS_UIOUT,
       			RS_DUMP	};

	perform;
};

#else	/* PICOS */

int mbeacon (word, address);

// ========================================= rcv

#define	RC_INIT		00
#define	RC_TRY		10
#define	RC_MSG		20

static int rcv_packet_size;
static char *rcv_buf_ptr;
static word rcv_rssi;

// ========================================= audit

#define	AS_INIT		00
#define	AS_START	10
#define	AS_TAGLOOP	20
#define	AS_TAGLOOP1	30
#define AS_HOLD		40

static lword lh_time;
static char *aud_buf_ptr;
static word aud_ind;

// ========================================= cmd_in

#define	CS_INIT		00
#define	CS_IN		10
#define	CS_WAIT		20

// ========================================= mbeacon

#define MB_START        00
#define MB_SEND		10

// ========================================= oss_out

#define	OO_RETRY	00

// ========================================= root

#define RS_INIT		00
#define RS_PAUSE	02
#define RS_FREE		10
#define RS_RCMD		20
#define RS_DOCMD	30
#define RS_UIOUT	40
#define RS_DUMP		50

#endif
#endif
