/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2004.                   */
/* All rights reserved.                                                 */
/* ==================================================================== */
#include "sysio.h"
#include "app.h"

extern tagArray[];
extern void init_tag (word);

void init_tags () {
	word i = tag_lim;
	while (i-- > 0)
		init_tag (i);
}

