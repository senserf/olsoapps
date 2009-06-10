#ifndef __node_h__
#define	__node_h__

#ifdef __SMURPH__

#include "board.h"

#include "app_types.h"
#include "applib_types.h"
#include "msg_types.h"
#include "sealists_types.h"

station SeaNode : PicOSNode {

// ============================================================================

#include "app_node_data.h"
#include "applib_node_data.h"
#include "msg_node_data.h"

// ============================================================================

#include "starter.h"

	void setup (data_no_t*);
	void reset ();
	void init ();
};

#define	_daprx(a)	_dac (SeaNode, a)

#endif
#endif
