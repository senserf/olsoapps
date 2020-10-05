/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#include "sysio.h"
#include "board.h"
#include "board.cc"

void buildErtNode (data_no_t*);
//void buildDumNode (data_no_t*);

process Root : BoardRoot {

	void buildNode (const char *tp, data_no_t *nddata) {
		if (strcmp (tp, "ert") == 0)
			buildErtNode (nddata);
//		else
//			buildDumNode (nddata);
	};
};
