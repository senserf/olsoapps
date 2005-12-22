/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2005                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "sysio.h"

heapmem {40, 60};

#include "ser.h"
#include "serf.h"
#include "form.h"

#define	MAX_N_CHUNKS	8

#define	RS_INIT		0
#define	RS_RCMD		10
#define	RS_ALL		20
#define	RS_DEA		30

address chunks [MAX_N_CHUNKS];

#if	MALLOC_SINGLEPOOL == 0
byte	pools [MAX_N_CHUNKS];
#endif

char	ibuf [48];

word	n, p, k;

address	chunk;

process (root, int)

  entry (RS_INIT)

  entry (RS_RCMD-2)

	ser_out (RS_RCMD-2,
		"\r\nMALLOC Test\r\n"
		"Commands:\r\n"
#if	MALLOC_SINGLEPOOL
		"a n k    -> allocate n bytes, chunk id is k\r\n"
#else
		"a n p k  -> allocate n bytes from pool p, chunk id is k\r\n"
#endif
		"d k      -> deallocate the chunk k\r\n"
		);

  entry (RS_RCMD)

	ser_in (RS_RCMD, ibuf, 48-1);

	if (ibuf [0] == 'a')
		proceed (RS_ALL);

	if (ibuf [0] == 'd')
		proceed (RS_DEA);

  entry (RS_RCMD+1)

	ser_out (RS_RCMD+1, "??\r\n");
	proceed (RS_RCMD-2);

  entry (RS_ALL)

#if	MALLOC_SINGLEPOOL
	scan (ibuf + 1, "%u %u", &n, &k);
	if (n == 0)
		proceed (RS_RCMD+1);
	if (k >= MAX_N_CHUNKS || chunks [k] != NULL)
		proceed (RS_RCMD+1);

	chunk = malloc (0, n);
	if (chunk == NULL) {
		diag ("FAILED");
		proceed (RS_RCMD);
	}
#else
	scan (ibuf + 1, "%u %u %u", &n, &p, &k);
	if (n == 0)
		proceed (RS_RCMD+1);
	if (k >= MAX_N_CHUNKS || chunks [k] != 0)
		proceed (RS_RCMD+1);

	chunk = malloc (p, n);
	if (chunk == NULL) {
		diag ("FAILED");
		proceed (RS_RCMD);
	}
#endif

	diag ("ALLOCATED at %x", chunk);
	chunks [k] = chunk;

#if	MALLOC_SINGLEPOOL == 0
	pools [k] = p;
#endif
	proceed (RS_RCMD);

  entry (RS_DEA)

	scan (ibuf + 1, "%u", &k);
	if (k >= MAX_N_CHUNKS || chunks [k] == NULL)
		proceed (RS_RCMD+1);

#if	MALLOC_SNGLEPOOL
	free (0, chunks [k]);
#else
	free (pools [k], chunks [k]);
#endif
	chunks [k] = NULL;
	diag ("DONE");
	proceed (RS_RCMD);

endprocess (1)
