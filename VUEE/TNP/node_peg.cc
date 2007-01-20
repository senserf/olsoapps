#include "diag.h"
#include "app_peg.h"
#include "msg_peg.h"

#include "node_peg.h"

void NodePeg::setup (data_no_t *nddata) {

	PicOSNode::setup (nddata);
	TNode::setup ();
	init ();
}

void NodePeg::init () {

#include "attribs_init_peg.h"

	// Start application root
	appStart ();
}

__PUBLF (NodePeg, void, reset) () {

	TNode::reset ();

	init ();
}

void buildPegNode (data_no_t *nddata) {
/*
 * The purpose of this is to isolate the two applications, such that their
 * specific "type" files don't have to be used together in any module.
 */
		create NodePeg (nddata);
}
