/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2012                           */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "app_tag.h"

fsm main_tag;

//  ECO (ARTURO* boards), ILS are here, handled by BOARD_ and PREINIT loading
//  board type into c_flags.

fsm root {

	entry INIT:
	runfsm main_tag;
	finish;

}
