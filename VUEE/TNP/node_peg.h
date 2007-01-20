#ifndef __node_peg_h__
#define	__node_peg_h__

#ifdef	__node_tag_h__
#error "node_tag.h and node_peg.h cannot be included together"
#endif

#include "board.h"
#include "chan_shadow.h"
#include "plug_tarp.h"

station	NodePeg : TNode {

	/*
	 * Session (application) specific data
	 */
#include "attribs_peg.h"

	/*
	 * Application starter
	 */
	void appStart ();

	void setup (data_no_t*);
	void _da (reset) ();
	void init ();
};

#endif
