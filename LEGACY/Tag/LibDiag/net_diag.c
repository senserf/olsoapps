/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2004.                   */
/* All rights reserved.                                                 */
/* ==================================================================== */
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
