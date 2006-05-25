/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2005.                   */
/* All rights reserved.                                                 */
/* ==================================================================== */
#include "sysio.h"
#include "lhold.h"

process (root, void)

	static lword lw = 120; 

	word w;

	entry (0)
		lw = (lword) (rnd () & 0x1ff);
		diag ("%d: lhold for %d", (word)seconds(), (word)lw);

	entry (1)
		lhold (1, &lw);
		diag ("%d: let go", (word)seconds ());
		proceed (0);

endprocess (0)



