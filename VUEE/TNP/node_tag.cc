#include "diag.h"
#include "app_tag.h"
#include "msg_tag.h"

#include "node_tag.h"

void NodeTag::setup (data_no_t* nddata) {

	PicOSNode::setup (nddata);
	TNode::setup ();
}

void NodeTag::init () {

#include "attribs_init_tag.h"

	// Start application root
	appStart ();
}

void NodeTag::reset () {

	TNode::reset ();
}

void buildTagNode (data_no_t* nddata) {
/*
 * The purpose of this is to isolate the two applications, such that their
 * specific "type" files don't have to be used together in any module.
 */
		create NodeTag (nddata);
}
