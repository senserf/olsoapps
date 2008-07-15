#ifndef	__picos_threadhdrs_h__
#define	__picos_threadhdrs_h__

#ifdef	__SMURPH__

process	THREADNAME (receiver) (Node) {

	address r_packet;

	states { RC_TRY, RC_DATA, RC_SACK, RC_ACK };

	perform;
};

process THREADNAME (sender) (Node) {

	address x_packet;
	word	data, packet_length;

	states { SN_SEND, SN_NEXT, SN_NEXT1 };

	perform;

	void setup (word del) {
		packet_length = 12;
		data = del;
	};
};

process THREADNAME (pin_monitor) (Node) {

	long CNT, CMP;
	word STA;
	const char *MSG;

	states {
		PM_START,
		PM_OUT,
		PM_NOTIFIER,
		PM_COUNTER,
		PM_EVENT,
		PM_EVENT1
	};

	perform;
};

process THREADNAME (root) (Node) {

	char *ibuf;
	int k, n1;
	const char *fmt;
	char obuf [32];
	word p [2];
	long lp;

	states {
		RS_INIT,
		RS_RCMDM2,
		RS_RCMDM1,
		RS_RCMD,
		RS_RCMD1,
		RS_SND,
		RS_SND1,
		RS_RCV,
		RS_QRCV,
		RS_QXMT,
		RS_QUIT,
		RS_QUIT1,
		RS_PAR,
		RS_PAR1,
		RS_SSID,
		RS_AUTOSTART,
		RS_RES,
		RS_RPIN,
		RS_RPIN1,
		RS_SPIN,
		RS_RANA,
		RS_RANA1,
		RS_RANA2,
		RS_WANA,
		RS_PSCN,
		RS_PSCM,
		RS_PGCN,
		RS_PGCN1,
		RS_PQCN,
		RS_PSNT,
		RS_PQNT,
		RS_PSMT,
		RS_PSMT1,
		RS_PQMT,
		RS_SETP,
		RS_GETP,
		RS_GETS,
		RS_GETS1,
		RS_GETS2,
		RS_SETA,
		RS_SETA1
	};

	perform;
};

#else

// ======================================

#define	RC_TRY		0
#define	RC_DATA		10
#define	RC_SACK		20
#define	RC_ACK		40

static address r_packet;

// ======================================

#define	SN_SEND		0
#define	SN_NEXT		10
#define	SN_NEXT1	20

static address 	x_packet;
static word	packet_length;

// ======================================

#define	PM_START	0
#define	PM_OUT		10
#define	PM_NOTIFIER	20
#define	PM_COUNTER	30
#define	PM_EVENT	40
#define	PM_EVENT1	50

static	word STA;
static	long CNT, CMP;
static	const char *MSG;

// ======================================

#define		RS_INIT		0
#define		RS_RCMDM2	10
#define		RS_RCMDM1	20
#define		RS_RCMD		30
#define		RS_RCMD1	40
#define		RS_SND		50
#define		RS_SND1		60
#define		RS_RCV		70
#define		RS_QRCV		80
#define		RS_QXMT		90
#define		RS_QUIT		100
#define		RS_QUIT1	110
#define		RS_PAR		120
#define		RS_PAR1		130
#define		RS_SSID		140
#define		RS_AUTOSTART	150
#define		RS_RES		160
#define		RS_RPIN		170
#define		RS_RPIN1	175
#define		RS_SPIN		180
#define		RS_RANA		190
#define		RS_RANA1	193
#define		RS_RANA2	196
#define		RS_WANA		197
#define		RS_PSCN		198
#define		RS_PSCM		199
#define		RS_PGCN		200
#define		RS_PGCN1	201
#define		RS_PQCN		202
#define		RS_PSNT		203
#define		RS_PQNT		204
#define		RS_PSMT		205
#define		RS_PSMT1	206
#define		RS_PQMT		207
#define		RS_SETP		210
#define		RS_GETP		220
#define		RS_GETS		230
#define		RS_GETS1	240
#define		RS_GETS2	250
#define		RS_SETA		260
#define		RS_SETA1	270
	
static	char *ibuf;
static	int k, n1;
static	const char *fmt;
static	char obuf [32];
static	word p [2];
static	long lp;

#endif
#endif
