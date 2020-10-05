/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#include "diag.h"
#include "app_peg.h"
#include "msg_peg.h"

#include "node_peg.h"

void NodePeg::init () {

#include "attribs_init_peg.h"

	// Start application root
	appStart ();
}

void buildPegNode (data_no_t *nddata) {
/*
 * The purpose of this is to isolate the two applications, such that their
 * specific "type" files don't have to be used together in any module.
 */
		create NodePeg (nddata);
}
