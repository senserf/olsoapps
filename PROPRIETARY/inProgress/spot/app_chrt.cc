/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2014                           */
/* All rights reserved.                                                 */
/* ==================================================================== */

#ifndef BOARD_CHRONOS
#error CHRONOS only
#endif

#include "chro_tag.h"

static void btyp_init () {
	chro_init();
}

#include "root_tag.cc"

