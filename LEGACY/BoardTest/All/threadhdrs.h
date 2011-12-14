#ifndef	__picos_threadhdrs_h__
#define	__picos_threadhdrs_h__

#ifdef	__SMURPH__

threadhdr (receiver, Node) {

	address r_packet;

	states { RC_TRY, RC_DATA };

	perform;
};

threadhdr (sender, Node) {

	address x_packet;

	states { SN_SEND, SN_MISS };

	perform;

};

threadhdr (root, Node) {

	char *ibuf;

	states {
		RS_INIT,
		RS_DONE,
		RS_TOUT,
		RS_OSS,
		RS_CMD
	};

	perform;
};

#else

// ======================================

#define	RC_TRY		0
#define	RC_DATA		10

static address r_packet;

// ======================================

#define	SN_SEND		0
#define	SN_MISS		10

static address 	x_packet;

// ======================================

#define		RS_INIT		0
#define		RS_DONE		10
#define		RS_TOUT		20
#define		RS_OSS		30
#define		RS_CMD		40
	
static	char *ibuf;

#endif
#endif
