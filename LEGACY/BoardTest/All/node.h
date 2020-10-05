/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#ifndef __node_h__
#define	__node_h__

#include "board.h"
#include "plug_null.h"

station	Node : PicOSNode {

	/*
	 * Session (application) specific data
	 */
#include "attribs.h"
#include "starter.h"

	void setup (data_no_t*);
	void reset ();
	void init ();
};

#endif
