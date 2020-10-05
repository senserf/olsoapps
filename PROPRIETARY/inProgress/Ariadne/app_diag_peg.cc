/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/

#include "diag.h"
#include "app_peg.h"
#include "msg_peg.h"
#include "form.h"
#include "app_peg_data.h"

void app_diag (const word level, const char * fmt, ...) {

	char * buf;
	va_list	ap;

	va_start (ap, fmt);

	if (app_dl < level)
		return;

	// compiled out if both levels are constant?
	// if not, go by #if DIAG_MESSAGES as well

	if ((buf = vform (NULL, fmt, ap)) == NULL) {
		diag ("no mem");
		return;
	}
	diag ("app_diag: %s", buf);
	ufree (buf);
}

void net_diag (const word level, const char * fmt, ...) {

	char * buf;
	va_list		ap;

	va_start (ap, fmt);

	if (net_dl < level)
		return;

	// compiled out if both levels are constant?

	if ((buf = vform (NULL, fmt, ap)) == NULL) {
		diag ("no mem");
		return;
	}
	diag ("net_diag: %s", buf);
	ufree (buf);
}
