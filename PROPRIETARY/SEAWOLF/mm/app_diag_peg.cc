/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2013.                   */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "diag.h"
#include "app_peg.h"
#include "msg_peg.h"
#include "form.h"
#include "app_peg_data.h"

void _app_diag (const char * fmt, ...) {

	char * buf;

	va_list		ap;
	va_start (ap, fmt);

	if ((buf = vform (NULL, fmt, ap)) == NULL) {
		diag ("no mem");
		return;
	}
	diag ("app: %s", buf);
	ufree (buf);
}

void _net_diag (const char * fmt, ...) {
	char * buf;

	va_list		ap;
	va_start (ap, fmt);

	if ((buf = vform (NULL, fmt, ap)) == NULL) {
		diag ("no mem");
		return;
	}
	diag ("net: %s", buf);
	ufree (buf);
}

