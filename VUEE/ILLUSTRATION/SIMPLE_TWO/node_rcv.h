#ifndef __node_rcv_h__
#define	__node_rcv_h__

#ifdef	__node_snd_h__
#error	"node_snd.h and node_rcv.h must not be included together!!!"
#endif

#include "board.h"

station NodeRcv : PicOSNode {

// ============================================================================

#include "app_rcv_data.h"

// ============================================================================

	void appStart ();
	// void setup (data_no_t*);
	void reset () { PicOSNode::reset (); };
	void init ();
};

#define	_daprx(a)	_dan (NodeRcv, a)

#endif
