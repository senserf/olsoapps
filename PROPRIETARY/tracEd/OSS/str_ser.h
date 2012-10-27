#ifndef __str_ser_h__
#define __str_ser_h__

// PiComp
//
// trueconst means a true constant, i.e., something that can be kept in code
// flash. For VUEE, this will be stored outside of node in a single copy for
// all nodes.

static trueconst char init_str[] = "%lu: Anybody? (Y)\r\n";

static trueconst char welcome_str[] =
	"TracEd commands:\r\n"
	"t [ dst [ dir [ hops ]]]\r\n"
	"s(tatus) [ dst ]\r\n"
	"a(ny) dst hops len <str>\r\n"
	"d(isp) len <str>\r\n"
	" <str>\r\n"
	"m(aster) [1/0]\r\n"
	"b(eacon) [ freq ]\r\n"
	"r(x) [1/0]\r\n"
	"h(elp)\r\n"
	"O(ff UI)\r\n"
	"q(uit)\r\n";

static trueconst char ill_str[] = "Failed (%s)\r\n";

static trueconst char stats_str[] =
	"Stats %u fl %x t %lu M %u m %u.%u s %u b %u\r\n";

static trueconst char beac_str[] = "Beac %u %u %u\r\n";

#endif
