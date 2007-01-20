#ifndef __node_tag_h__
#define	__node_tag_h__

#ifdef	__node_peg_h__
#error "node_tag.h and node_peg.h cannot be included together"
#endif

#include "board.h"
#include "chan_shadow.h"
#include "plug_tarp.h"

station	NodeTag : TNode {

	/*
	 * Session (application) specific data
	 */
#include "attribs_tag.h"

	/*
	 * Application starter
	 */
	void appStart ();

	void setup (data_no_t*);
	void _da (reset) ();
	void init ();
};

#endif
