#ifndef	__picos_threadhdrs_h__
#define	__picos_threadhdrs_h__

#ifdef	__SMURPH__

process	THREADNAME (receiver) (Node) {

	address	r_packet;

	states { RC_TRY, RC_SHOW };

	perform;
};

process THREADNAME (sender) (Node) {

	char *data;

	states { SN_SEND };

	void setup (char *s) {
		data = s;
	};

	perform;
};

process THREADNAME (root) (Node) {

	char *ibuf;

	states {
		RS_INIT,
		RS_RCMD_M,
		RS_RCMD,
		RS_RCMD_E,
		RS_XMIT
	};

	perform;
};

#else

// ======================================

#define	RC_TRY		0
#define	RC_SHOW		1

address r_packet;

// ======================================

#define	SN_SEND		0

// ======================================

#define	RS_INIT		0
#define	RS_RCMD_M	1
#define	RS_RCMD		2
#define	RS_RCMD_E	3
#define	RS_XMIT		4

char *ibuf;

#endif
#endif
