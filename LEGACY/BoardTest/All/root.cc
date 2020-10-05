/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#include "node.h"

#include "board.cc"

process Root : BoardRoot {

	void buildNode (const char *tp, data_no_t *nddata) {
		// Type ignored; we only handle a single type in this case
		create Node (nddata);
	};
};
