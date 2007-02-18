#ifndef __node_h__
#define	__node_h__

#include "board.h"
#include "chan_shadow.h"
#include "plug_null.h"

station	Node : NNode {

	/*
	 * Session (application) specific data
	 */
#include "attribs.h"
#include "starter.h"

	void setup (data_no_t*);
	void _da (reset) ();
	void init ();
};

#endif
