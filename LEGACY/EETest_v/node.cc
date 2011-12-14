#include "node.h"

void Node::setup (data_no_t *nddata) {

	PicOSNode::setup (nddata);
}

void Node::init () {

#include "attribs_init.h"

	// Start application root
	appStart ();
}

void Node::reset () {

	PicOSNode::reset ();
}
