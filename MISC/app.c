/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2005.                   */
/* All rights reserved.                                                 */
/* ==================================================================== */
#include "sysio.h"
#include "lhold.h"

static lword lw;
static int pcs;

process (sleeper, void)

	entry (0)
		lw = (lword) (rnd () & 0x1ff);
		diag ("%d: lhold for %d", (word)seconds(), (word)lw);

	entry (1)
		lhold (1, &lw);
		diag ("%d: let go", (word)seconds ());
		proceed (0);

endprocess (0)

process (root, void)

	entry (0)

		pcs = fork (sleeper, NULL);

	entry (1)

		diag ("%d: left %d", (word) seconds (),
			(word) lhleft (pcs, &lw));
		delay (((rnd () & 0xf) + 1) << 10, 1);

endprocess (0)
