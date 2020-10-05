/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#ifndef __node_tag_h__
#define	__node_tag_h__

#ifdef	__node_peg_h__
#error "node_tag.h and node_peg.h cannot be included together"
#endif

#include "board.h"
#include "plug_tarp.h"

station	NodeTag : PicOSNode {

	/*
	 * Session (application) specific data
	 */
#include "attribs_tag.h"

	/*
	 * Application starter
	 */
	void appStart ();

	void setup (data_no_t*);
	void reset ();
	void init ();
};

#endif
