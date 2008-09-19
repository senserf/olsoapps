#ifndef __globals_peg_h__
#define __globals_peg_h__

#include "app_peg.h"
#include "msg_peg.h"
#include "diag.h"

#ifdef	__SMURPH__

#include "node_peg.h"
#include "stdattr.h"

#else	/* PICOS */

#include "form.h"
#include "serf.h"
#include "ser.h"
#include "net.h"
#include "tarp.h"

#include "attribs_peg.h"

heapmem {80, 20}; // how to find out a good ratio?

extern const lword host_id;
//+++ "hostid.c"

// from, to, current, and for set_rf (sort of previous, on the Master only)
sattr_t sattr[4];

tout_t touts = { 0, 0, {(DEF_I_SIL << 12) | (DEF_I_WARM << 8) | DEF_I_CYC} };

char	*ui_ibuf	= NULL,
	*ui_obuf	= NULL,
	*cmd_line	= NULL;

word	app_flags 	= DEF_APP_FLAGS;
word	sstate		= ST_OFF; // waste of 12 bits
word	oss_stack	= 0;
word	cters[3]	= {0, 0, 0};

#endif

#include "attnames_peg.h"

// These are static const and can thus be shared

static const char welcome_str[] = "***Site Survey 0.2***\r\n"
	"Set / show attribs:\r\n"
	"\tfrom:\t\tsf [pl [ra [ch]]]\r\n"
	"\tto:\t\tst [pl [ra [ch]]]\r\n"
	"\tid:\t\tsi x y <new x> <new y>\r\n"
	"\tuart:\t\tsu [0 | 1]\r\n"
	"\tfmt:\t\tsy [0 | 1]\r\n"
	"\tall out:\tsa [0 | 1]\r\n"
	"\tstats:\t\ts [x y]\r\n\r\n"
	"Timeouts / intervals:\r\n"
	"\tt [<silence> [<warmup> [<cycle>]]]\r\n\r\n"
	"Execute Operation:\r\n"
	"\tsilence:\tos\r\n"
	"\tgo:\t\tog\r\n"
	"\tmaster:\t\tom\r\n"
	"\tauto:\t\toa\r\n"
	"\terase:\t\toe\r\n\r\n"
	"Help: \th\r\n";

static const char ill_str[] =	"Illegal command (%s)\r\n";
static const char bad_str[] =   "Bad or incomplete command (%s)\r\n";
static const char bad_state_str[] = "Not allowed in state (%u)\r\n";

static const char stats_str[] = "Stats for %s hid - lh %lx - %u (%u, %u):\r\n"
	" Attr pl %u-%u (%u), rate %u-%u (%u), ch %u-%u (%u)\r\n"
	" Counts m(%u) r(%u) t(%u) u(%u), touts: sil %u, warm %u, cyc %u\r\n"
	" Time %lu (sstart %lu), Flags %x, Mem: free %u, min %u\r\n"
	" State %u RF %u\r\n";

static const char ping_ascii_def[] = "Ping %lu\r\n"
	"[%u %u %u] %u (%u) (%u, %u)->(%u, %u)\r\n"
	"[%u %u %u] %u (%u) (%u, %u)->(%u, %u)\r\n";

static const char ping_ascii_raw[] = " %lu %u %u %u %u %u %u %u %u %u"
	" %u %u %u %u %u %u %u %u %u\r\n";

#endif
