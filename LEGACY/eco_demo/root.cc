/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#include "common.h"
#include "board.cc"

process Root : BoardRoot {

	void buildNode (const char *tp,	data_no_t *nddata) {

		if (strcmp (tp, "tag") == 0)
			buildTagNode (nddata);
		else if (strcmp (tp, "peg") == 0)
			buildPegNode (nddata);
		else if (strcmp (tp, "cus") == 0)
			buildCusNode (nddata);
		else
			excptn ("Root: illegal node type: %s", tp);
	};
};
