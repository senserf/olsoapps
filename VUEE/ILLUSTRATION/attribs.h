#ifndef	__praxis_attribs_h__
#define	__praxis_attribs_h__
#ifdef	__SMURPH__
// This is applicable only to SMURPH models (in case the praxis includes this
// file accidentally)

	int	_da (sfd);
	word	_da (Count);
	void	_da (show) (word, address);
	word	_da (plen) (char*);
#endif
#endif
