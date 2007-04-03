#ifndef	__praxis_attribs_h__
#define	__praxis_attribs_h__

#ifdef	__SMURPH__
// This is applicable only to SMURPH models (in case the praxis includes this
// file accidentally)

	lword	_da (last_snt);
	lword	_da (last_rcv) [MAX_PEER_COUNT];
	lword	_da (received) [MAX_PEER_COUNT];
	lword	_da (lost) [MAX_PEER_COUNT];
	word	_da (packet_length);
	word	_da (send_interval);

	int	_da (sfd);
	word	_da (NODE_ID);

	Boolean	_da (receiving);

#endif

#endif
