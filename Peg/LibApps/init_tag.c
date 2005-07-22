/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2004.                   */
/* All rights reserved.                                                 */
/* ==================================================================== */
#include "sysio.h"
#include "app.h"
#include "lib_apps.h"

void init_tag (word i) {
	tagArray[i].id = 0;
	tagArray[i].state = noTag;
	tagArray[i].count = 0;
	tagArray[i].evTime = 0;
	tagArray[i].lastTime = 0;
}

