#ifndef	__praxis_attribs_h__
#define	__praxis_attribs_h__
#ifdef	__SMURPH__
// This is applicable only to SMURPH models (in case the praxis includes this
// file accidentally)

	int	_da (sfd), _da (last_snt), _da (last_rcv), _da (last_ack);
	Boolean	_da (XMTon), _da (RCVon), _da (rkillflag), _da (tkillflag);

	int _da (rcv_start) ();
	int _da (rcv_stop) ();
	int _da (snd_start) (int);
	int _da (snd_stop) ();
#endif
#endif
