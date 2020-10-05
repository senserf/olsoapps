/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/

#include "app_tag.h"

fsm main_tag;

//  ECO (ARTURO* boards), ILS are here, handled by BOARD_ and PREINIT loading
//  board type into c_flags.

fsm root {

	entry INIT:
	runfsm main_tag;
	finish;

}
