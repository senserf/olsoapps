/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2010                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "vuee_peg.h"
#include "diag.h"
#include "app_peg.h"
#include "msg_peg.h"
#include "oss_fmt.h"

#ifdef	__SMURPH__

//#include "stdattr.h"

#else	/* PICOS */

#include "form.h"
#include "oss_fmt.h"

#endif	/* SMURPH or PICOS */

void app_diag (const word level, const char * fmt, ...) {

	char * buf;

#ifdef	__SMURPH__
#define	va_par(s)	ap
	va_list		ap;
	va_start (ap, fmt);
#endif

	if (app_dl < level)
		return;

	// compiled out if both levels are constant?
	// if not, go by #if DIAG_MESSAGES as well

	if ((buf = vform (NULL, fmt, va_par (fmt))) == NULL) {
		diag (OPRE_DIAG "no mem");
		return;
	}
	diag (OPRE_DIAG "L%u: %s", level, buf);
	ufree (buf);
}

void net_diag (const word level, const char * fmt, ...) {
	char * buf;

#ifdef	__SMURPH__
	va_list		ap;
	va_start (ap, fmt);
#endif

	if (net_dl < level)
		return;

	// compiled out if both levels are constant?

	if ((buf = vform (NULL, fmt, va_par (fmt))) == NULL) {
		diag (OPRE_DIAG "no mem");
		return;
	}
	diag (OPRE_DIAG "L%u: %s", level, buf);
	ufree (buf);
}
