/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#include "form.h"
#include "diag.h"
void net_diag (const word level, const char * fmt, ...) {
	char * buf;

	if (net_dl < level)
		return;

	// compiled out if both levels are constant?

	buf = vform (NULL, fmt, va_par (fmt));
	diag ("net_diag: %s", buf);
	ufree (buf);
}
