/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#ifndef __node_h__
#define	__node_h__

#ifdef	__SMURPH__

#include "board.h"

station Node : PicOSNode {

// ============================================================================

#include "app_data.h"

// ============================================================================


	void appStart ();

	void init () {

#include "app_data_init.h"

		appStart ();
	};
};

#include "stdattr.h"

#endif
#endif
