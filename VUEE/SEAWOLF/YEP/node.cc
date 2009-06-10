#include "vuee.h"

void SeaNode::setup (data_no_t *nddata) { PicOSNode::setup (nddata); }

void SeaNode::init () {

// ============================================================================

#include "app_node_data_init.h"
#include "applib_node_data_init.h"
#include "msg_node_data_init.h"

// ============================================================================

	// Start application root
	appStart ();
}

#undef reset

void SeaNode::reset () { PicOSNode::reset (); }
