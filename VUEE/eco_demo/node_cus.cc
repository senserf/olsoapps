#include "diag.h"
#include "app_cus.h"
#include "msg_peg.h"

#include "node_cus.h"

void NodeCus::setup (data_no_t *nddata) {

	PicOSNode::setup (nddata);
	TNode::setup ();
}

void NodeCus::init () {

#include "attribs_init_cus.h"

	// Start application root
	appStart ();
}

void NodeCus::reset () {

	TNode::reset ();
}

void buildCusNode (data_no_t *nddata) {
/*
 * The purpose of this is to isolate the two applications, such that their
 * specific "type" files don't have to be used together in any module.
 */
		create NodeCus (nddata);
}
