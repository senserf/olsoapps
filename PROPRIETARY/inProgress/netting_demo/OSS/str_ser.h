/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#ifndef __str_ser_h__
#define __str_ser_h__

// PiComp
//
// trueconst means a true constant, i.e., something that can be kept in code
// flash. For VUEE, this will be stored outside of node in a single copy for
// all nodes.

static trueconst char welcome_str[] = 
	"Netting commands:\r\n\r\n"
	"s (set / show) disp, odr, trace, beacon, TARP:\r\n"
	"sd [ref# dst ack <string>]\r\n"
	"so [ref# ack hop1 ... hop9]\r\n"
	"st [ref# [dst(0) [ dir(3) [ hops(0) ]]]]\r\n"
	"sb [ (d,o,t) freq (<64) volume (>0) ]\r\n"
	"sT *CAREFUL* [plev rx fwd slack rte tlev]\r\n"
	"s (show node status)\r\n\r\n"
	"Beacon operations:\r\n"
	"ba (activate)\r\n"
	"bd (deactivate)\r\n\r\n"
	"Transmit:\r\n"
	"x(d,o,t)\r\n\r\n"
	"FIM operations:\r\n"
	"Fw(rite)\r\n"
	"Fe(rase)\r\n"
	"F (read)\r\n\r\n"
	"m(aster) [1/0]\r\n"
	"t(ransient) [1/0]\r\n"
	"f(ormat) [1/0]\r\n"
	"h(elp)\r\n"
	"q(uit)\r\n";

static trueconst char ill_str[] = "Failed (%s)\r\n";

static trueconst char notrun_str[] = "Not running\r\n";

static trueconst char alrun_str[] = "Already running\r\n";

static trueconst char notset_str[] = "Not set\r\n";

static trueconst char lck_str[] = "Locked\r\n";

static trueconst char stats_str[] =
	"%u(%u) at %lu: M %u at %lu mem %u.%u.%u bat %u trans %u ofmt %u\r\n";

static trueconst char tarp_str[] =
	"Plev %u rx %u fwd %u slack %u rte %u tlev %u\r\n";

static trueconst char beac_str[] = "Beac %c(%u) freq %u %u of %u\r\n";

static trueconst char disp_str[] = "Disp #%lu to %u ack %u len %u <%s>\r\n";

static trueconst char odr_str[] = "Odr(%u) #%lu ack %u "
					"[%u %u %u %u %u %u %u %u %u]\r\n";

static trueconst char trac_str[] = "Trace #%lu to %u dir %u hco %u\r\n";

static trueconst char fim_str[] = "FIM(%u): plev %u rx %u fwd %u slack %u"
					" rte %u tlev %u\r\n";
 
#endif
