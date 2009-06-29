#ifndef __node_snd_h__
#define	__node_snd_h__

#ifdef	__node_rcv_h__
#error	"node_snd.h and node_rcv.h must not be included together!!!"
#endif

#include "board.h"

station NodeSnd : PicOSNode {

// ============================================================================

#include "app_snd_data.h"

// ============================================================================

	void appStart ();
	void reset () { PicOSNode::reset (); };
	void init ();
};

#define	_daprx(a)	_dan (NodeSnd, a)

#endif
