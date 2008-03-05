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

	word aud_ind;

	states 	{ AS_START, AS_TAGLOOP };

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

	word	rt_ind;

	states { RS_INIT, RS_PAUSE, RS_FREE, RS_RCMD, RS_DOCMD, RS_UIOUT,
       		 RS_SETS, RS_PROFILES, RS_HELPS, RS_LISTS, RS_Y, RS_N,
		 RS_TARGET, RS_BIZ, RS_PRIV, RS_ALRM, RS_STOR, RS_RETR,
		 RS_BEAC, RS_L_TAG, RS_L_IGN, RS_L_MON, RS_MLIST };

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

#define	AS_START	0
#define	AS_TAGLOOP	10

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

#define RS_INIT		0
#define RS_PAUSE	1
#define RS_FREE		2
#define RS_RCMD		3
#define RS_DOCMD	4
#define RS_UIOUT	5
#define RS_SETS		6
#define RS_PROFILES	7
#define RS_HELPS	8
#define RS_LISTS	9
#define RS_Y		10
#define RS_N		11
#define RS_TARGET	12
#define RS_BIZ		13
#define RS_PRIV		14
#define RS_ALRM		15
#define RS_STOR		16
#define RS_RETR		17
#define RS_BEAC		18
#define RS_L_TAG	19
#define RS_L_IGN	20
#define RS_L_MON	21
#define RS_MLIST	22

static word	rt_ind;

#endif
#endif
