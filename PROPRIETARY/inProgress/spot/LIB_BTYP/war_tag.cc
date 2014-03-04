/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2011-2014                      */
/* All rights reserved.                                                 */
/* ==================================================================== */
#include "war_tag.h"

// because of set_alrm():
#include "variants.h"

war_t	warsaw; // my state

fsm randal () {
	state INI:
	    delay ((50 + (rnd() % 10)) << 10, ALRM);
	    release;
	state ALRM:
	    word w = (word)rnd();
	    if (w & 1) // further .5 chance
		set_alrm (7);

	    proceed INI;
}

void war_init () {
	runfsm randal;
}

