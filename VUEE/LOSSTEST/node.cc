#include "node.h"

void Node::setup (data_no_t *nddata) {

	PicOSNode::setup (nddata);
	NNode::setup ();
	init ();
}

void Node::init () {

#include "attribs_init.h"

	// Start application root
	appStart ();
}

__PUBLF (Node, void, reset) () {

	NNode::reset ();

	init ();
}
