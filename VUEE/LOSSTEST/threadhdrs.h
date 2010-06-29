#ifndef	__picos_threadhdrs_h__
#define	__picos_threadhdrs_h__

#ifdef	__SMURPH__

threadhdr (receiver, Node) {

	word	r_sn, r_pl;
	byte	r_sl;

	states { RC_TRY, RC_OUT };

	perform;
};

threadhdr (sender, Node) {

	states { SN_SEND, SN_OUT };

	perform;

};

threadhdr (root, Node) {

	char *t_ibuf;
	word t_i;

	states { RS_INIT, RS_MENU, RS_RCMD, RS_OK, RS_SP, RS_CNT };

	perform;
};

#else

// ======================================

#define	RC_TRY		0
#define	RC_OUT		1

static	word	r_sn, r_pl;
static	byte	r_sl;

// ======================================

#define	SN_SEND		0
#define	SN_OUT		1

// ======================================

#define		RS_INIT		0
#define		RS_MENU		1
#define		RS_RCMD		2
#define		RS_OK		3
#define		RS_SP		4
#define		RS_CNT		5
	
static	char *t_ibuf;
static	word t_i;

#endif
#endif
