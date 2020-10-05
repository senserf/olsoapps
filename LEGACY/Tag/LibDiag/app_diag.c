/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#include "form.h"
#include "diag.h"
void app_diag (const word level, const char * fmt, ...) {
	char * buf;

	if (app_dl < level)
		return;

	// compiled out if both levels are constant?
	// if not, go by #if DIAG_MESSAGES as well

	buf = vform (NULL, fmt, va_par (fmt));
	diag ("app_diag: %s", buf);
	ufree (buf);
}
