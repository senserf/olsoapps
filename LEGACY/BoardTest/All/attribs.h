/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#ifndef	__praxis_attribs_h__
#define	__praxis_attribs_h__
#ifdef	__SMURPH__
// This is applicable only to SMURPH models (in case the praxis includes this
// file accidentally)

	lword	_da (my_id);
	sint	_da (sfd);
	word	_da (last_snt), _da (last_run), _da (rf_start), _da (max_rss),
		_da (plev);
#endif
#endif
