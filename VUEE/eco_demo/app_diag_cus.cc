/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2008.                   */
/* All rights reserved.                                                 */
/* ==================================================================== */

#ifndef	__app_diag_cus_h__
#define	__app_diag_cus_h___

#include "diag.h"
#include "app_cus.h"
#include "msg_peg.h"
#include "oss_fmt.h"

#ifdef	__SMURPH__

#include "node_cus.h"
#include "stdattr.h"
#include "attnames_cus.h"

#else	/* PICOS */

#include "form.h"
#include "oss_fmt.h"

#endif	/* SMURPH or PICOS */

__PUBLF (NodeCus, void, app_diag) (const word level, const char * fmt, ...) {

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

__PUBLF (NodeCus, void, net_diag) (const word level, const char * fmt, ...) {
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

#endif
