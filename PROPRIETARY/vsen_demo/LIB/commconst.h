/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#ifndef __commconst_h_
#define __commconst_h_

// MAX_SENS is the largest of NUM_SENS's... to make it better, a lot of
// things would have to be decoupled between pegs and tags, but variable msg
// length is definitely a good thing. It is not so with EEPROM - I don't think
// variable lengths are feasible, e.g. bisections?

// MAX/NUM_SENS 9 is max. for 60B packet length. If slightly more is needed,
// we may change content of reports, e.g. remove slots, tstamps.

#define MAX_SENS	8

#endif
