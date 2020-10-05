/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
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
