#ifndef __node_cus_h__
#define	__node_cus_h__

#ifdef	__node_peg_h__
#error "node_cus.h node_peg.h cannot be included together"
#endif

#ifdef  __node_tag_h__
#error "node_tag.h node_cus.h cannot be included together"
#endif

#include "board.h"
#include "plug_tarp.h"

station	NodeCus : PicOSNode {

	/*
	 * Session (application) specific data
	 */
#include "attribs_cus.h"

	/*
	 * Application starter
	 */
	void appStart ();
	void init ();
};

#endif