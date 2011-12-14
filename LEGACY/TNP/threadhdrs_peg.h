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

	char *aud_buf_ptr;
	word aud_ind;

	states 	{ AS_INIT, AS_START, AS_TAGLOOP, AS_TAGLOOP1 };

	perform;

};

threadhdr (cmd_in, NodePeg) {

	states { CS_INIT, CS_IN, CS_WAIT };

	perform;
};

strandhdr (oss_out, NodePeg) {

	char *data;

	states { OO_RETRY };

	void setup (char *d) { data = d; };

	perform;
};

threadhdr (root, NodePeg) {

	lword	rtin_tag, rtin_pass, rtin_npass;
	nid_t	rtin_peg;
	word	rtin_rssi, rtin_pl, rtin_maj, rtin_min, rtin_span;

	states { RS_INIT, RS_FREE, RS_RCMD, RS_DOCMD, RS_UIOUT };

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

static char *aud_buf_ptr;
static word aud_ind;

// ========================================= cmd_in

#define	CS_INIT		00
#define	CS_IN		10
#define	CS_WAIT		20

// ========================================= oss_out

#define	OO_RETRY	00

// ========================================= root

#define RS_INIT		00
#define RS_FREE		10
#define RS_RCMD		20
#define RS_DOCMD	30
#define RS_UIOUT	40

static lword	rtin_tag, rtin_pass, rtin_npass;
static nid_t	rtin_peg;
static word	rtin_rssi, rtin_pl, rtin_maj, rtin_min, rtin_span;

#endif
#endif
