#ifndef	__threadhdrs_cus_h__
#define	__threadhdrs_cus_h__

#ifdef	__SMURPH__

threadhdr (rcv, NodeCus) {

	int	rcv_packet_size;
	char 	*rcv_buf_ptr;
	word 	rcv_rssi;

	states 	{ RC_INIT, RC_TRY, RC_MSG };

	perform;
};

threadhdr (audit, NodeCus) {

	lword lh_time;
	char *aud_buf_ptr;
	word aud_ind;

	states 	{ AS_INIT, AS_START, AS_TAGLOOP, AS_TAGLOOP1, AS_HOLD };

	perform;

};

threadhdr (cmd_in, NodeCus) {

	states { CS_INIT, CS_IN, CS_WAIT };

	perform;
};

strandhdr (oss_out, NodeCus) {

	char *data;

	states { OO_RETRY };

	void setup (char *d) { data = d; };

	perform;
};

threadhdr (root, NodeCus) {

	states { RS_INIT, RS_INIT1, RS_INIT2, RS_PAUSE, RS_FREE, RS_RCMD,
	       	RS_DOCMD, RS_UIOUT, RS_DUMP };

	perform;
};

#else	/* PICOS */

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

// ========================================= oss_out

#define	OO_RETRY	00

// ========================================= root

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
#endif
