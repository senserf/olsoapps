/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2005.                   */
/* All rights reserved.                                                 */
/* ==================================================================== */
#include "sysio.h"
#include "lhold.h"

word when [10];

process (sleeper, int)

#define	PNUM	data

	word	dd, ns;

	entry (0)
		// Generate a number of seconds to sleep
		dd = rnd () & 0x3f;
		ns = (word) seconds ();
		diag ("%u: [%d] for %d sec", ns, PNUM, dd);
		when [PNUM] = ns + dd;
		delay (dd << 10, 1);
		release;

	entry (1)
		ns = (word) seconds ();
		diag ("%u: [%d] wake up %u", ns, PNUM, when [PNUM]);
		proceed (0);

endprocess

process (root, void*)

	int i;

	entry (0)

		for (i = 0; i < 8; i++)
			fork (sleeper, i);

	entry (1)

		delay (10 << 10, 2);
		release;

	entry (2)

		diag ("Freezing");
		freeze (4);
		diag ("Unfreezing");
		proceed (1);

endprocess
