/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2004.                   */
/* All rights reserved.                                                 */
/* ==================================================================== */

#ifndef	__app_diag_peg_h__
#define	__app_diag_peg_h___

#include "diag.h"
#include "app_peg.h"
#include "msg_peg.h"

#ifdef	__SMURPH__

#include "node_peg.h"
#include "stdattr.h"
#include "attnames_peg.h"

#else	/* PICOS */

#include "form.h"

#endif	/* SMURPH or PICOS */

__PUBLF (NodePeg, void, app_diag) (const word level, const char * fmt, ...) {

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
		diag ("no mem");
		return;
	}
	diag ("app_diag: %s", buf);
	ufree (buf);
}

__PUBLF (NodePeg, void, net_diag) (const word level, const char * fmt, ...) {
	char * buf;

#ifdef	__SMURPH__
	va_list		ap;
	va_start (ap, fmt);
#endif

	if (net_dl < level)
		return;

	// compiled out if both levels are constant?

	if ((buf = vform (NULL, fmt, va_par (fmt))) == NULL) {
		diag ("no mem");
		return;
	}
	diag ("net_diag: %s", buf);
	ufree (buf);
}

#endif
