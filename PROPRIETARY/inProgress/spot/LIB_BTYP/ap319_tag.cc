/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#include "ap319_tag.h"
#include "alrms.h"

ap319_t	ap319;

#ifdef  __SMURPH__

static word sw = (word) preinit ("switches");

void ap319_rsw () {
        ap319.dial = (byte)((sw & 0xF) + ((sw >> 4) & 0xF) * 10);
        ap319.gmap = (byte)(sw >> 8);
}

#else

#include "switches.h"

void ap319_rsw () {
        switches_t sw;
        read_switches (&sw);
        ap319.dial = (byte)(sw.S0 + sw.S1 * 10);
        ap319.gmap = sw.S2;
}
#endif

static void do_butt (word b) {
	set_alrm (b +1); // buttons 0.., alrms 1..
}

void ap319_init () {
	buttons_action (do_butt);
}
