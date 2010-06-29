#ifndef	__threadhdrs_peg_h__
#define	__threadhdrs_peg_h__

#ifdef	__SMURPH__

threadhdr (rcv, NodePeg) {

	int	rcv_packet_size;
	char 	*rcv_buf_ptr;
	word 	rcv_rssi;

	states 	{ RC_INIT, RC_TRY, RC_MSG, RC_SIL };

	perform;
};

threadhdr (m_cyc, NodePeg) {

	states 	{ MC_INIT, MC_LOOP, MC_MBEAC, MC_SEED, MC_CYC, MC_NEXT,
       		  MC_MBEAC2 };

	perform;

};

threadhdr (cmd_in, NodePeg) {

	states { CS_INIT, CS_IN, CS_WAIT };

	perform;
};

threadhdr (m_sil, NodePeg) {

	states { MS_INIT, MS_SBEAC, MS_SBEAC2 };

	perform;
};

strandhdr (oss_out, NodePeg) {

	char *data;

	states { OO_INIT, OO_RETRY };

	void setup (char *d) { data = d; };

	perform;
};

threadhdr (root, NodePeg) {

	states { RS_INIT, RS_FREE, RS_RCMD, RS_DOCMD, RS_UIOUT,
       		 RS_SETS, RS_TOUTS, RS_OPERS, RS_WARMUP, RS_RCMD2 };

	perform;
};

#else	/* PICOS */

//int mbeacon (word, address);

// ========================================= rcv

#define	RC_INIT		00
#define	RC_TRY		10
#define	RC_MSG		20
#define RC_SIL		30

static int rcv_packet_size;
static char *rcv_buf_ptr;
static word rcv_rssi;

// ========================================= audit

#define	MC_INIT		0
#define	MC_LOOP		1
#define MC_MBEAC	2
#define MC_SEED		3
#define MC_CYC		4
#define MC_NEXT		5
#define MC_MBEAC2	6

// ========================================= cmd_in

#define	CS_INIT		00
#define	CS_IN		10
#define	CS_WAIT		20

// ========================================= mbeacon

#define MS_INIT		0
#define MS_SBEAC	10
#define MS_SBEAC2	20

// ========================================= oss_out

#define	OO_INIT		0
#define OO_RETRY	1

// ========================================= root

#define RS_INIT		0
#define RS_FREE		1
#define RS_RCMD		2
#define RS_DOCMD	3
#define RS_UIOUT	4
#define RS_SETS		5
#define RS_TOUTS	6
#define RS_OPERS	7
#define RS_WARMUP	8
#define RS_RCMD2	9

#endif
#endif
