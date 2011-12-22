/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2008.                   */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "diag.h"
#include "app_cus.h"
#include "msg_peg.h"
#include "oss_fmt.h"
#include "form.h"
#include "oss_fmt.h"
#include "app_cus_data.h"

idiosyncratic void app_diag (const word level, const char * fmt, ...) {

	char * buf;

	va_list		ap;
	va_start (ap, fmt);

	if (app_dl < level)
		return;

	// compiled out if both levels are constant?
	// if not, go by #if DIAG_MESSAGES as well

	if ((buf = vform (NULL, fmt, ap)) == NULL) {
		diag (OPRE_DIAG "no mem");
		return;
	}
	diag (OPRE_DIAG "L%u: %s", level, buf);
	ufree (buf);
}

idiosyncratic void net_diag (const word level, const char * fmt, ...) {

	char * buf;

	va_list		ap;
	va_start (ap, fmt);

	if (net_dl < level)
		return;

	// compiled out if both levels are constant?

	if ((buf = vform (NULL, fmt, ap)) == NULL) {
		diag (OPRE_DIAG "no mem");
		return;
	}
	diag (OPRE_DIAG "L%u: %s", level, buf);
	ufree (buf);
}
