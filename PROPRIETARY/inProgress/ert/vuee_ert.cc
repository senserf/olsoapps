/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#include "app_ert.h"
#include "vuee_ert.h"

void NodeErt::init () {

// ============================================================================

#define	__dcx_ini__
#include "app_ert_data.h"
#undef	__dcx_ini__

// ============================================================================

	// Start application root
	appStart ();
}

void buildErtNode (data_no_t *nddata) {

	create NodeErt (nddata);
}
