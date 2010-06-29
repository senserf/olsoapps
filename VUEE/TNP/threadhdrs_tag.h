#ifndef	__threadhdrs_tag_h__
#define	__threadhdrs_tag_h__

#ifdef	__SMURPH__

threadhdr (rcv, NodeTag) {

	int	rcv_packet_size;
	char 	*rcv_buf_ptr;

	states 	{ RC_INIT, RC_TRY, RC_MSG };

	perform;
};

threadhdr (rxsw, NodeTag) {

	states 	{ RX_OFF, RX_ON };

	perform;

};

threadhdr (pong, NodeTag) {

	char	png_frame [sizeof (msgPongType)];
	word	png_shift;

	states 	{ PS_INIT, PS_NEXT, PS_SEND, PS_SEND1 };

	perform;
};

threadhdr (cmd_in, NodeTag) {

	states { CS_INIT, CS_IN, CS_WAIT };

	perform;
};

threadhdr (root, NodeTag) {

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

// ========================================= rxsw

#define	RX_OFF		00
#define	RX_ON		10

// ========================================= pong

static char	 	png_frame [sizeof (msgPongType)];
static word 		png_shift;

#define	PS_INIT		00
#define	PS_NEXT		10
#define	PS_SEND		20
#define	PS_SEND1	30

// ========================================= cmd_in

#define	CS_INIT		00
#define	CS_IN		10
#define	CS_WAIT		20

// ========================================= root

#define RS_INIT		00
#define RS_FREE		10
#define RS_RCMD		20
#define RS_DOCMD	30
#define RS_UIOUT	40

#endif
#endif
