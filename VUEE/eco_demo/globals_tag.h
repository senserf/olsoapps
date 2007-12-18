#ifndef __globals_tag_h__
#define __globals_tag_h__

#define TINY_MEM 1

#include "app_tag.h"
#include "msg_tag.h"
#include "diag.h"

#ifdef	__SMURPH__

#include "node_tag.h"
#include "stdattr.h"

#else	/* PICOS */

#include "form.h"
#include "ser.h"
#include "net.h"
#include "tarp.h"

#include "attribs_tag.h"

heapmem {80, 20}; // how to find out a good ratio?

char * ui_ibuf 	= NULL;
char * ui_obuf 	= NULL;
char * cmd_line	= NULL;

const   lword	host_id		= 0xBACA0065;

word		app_flags	= 0;

pongParamsType	pong_params = {	60, 	// freq_maj in sec, max 63K
				5,  	// freq_min in sec. max 63
				0x7777, // levels / retries
				1024, 	// rx_span in msec (max 63K) 1: ON
				0,	// rx_lev: select if needed, 0: all
				0	// pload_lev: same
};

sensDataType	sens_data;
long		lh_time;
long		ref_time;
sensEEDumpType	*sens_dump = NULL;
#endif

#include "attnames_tag.h"

// These are static const and can thus be shared

#if TINY_MEM
static const char welcome_str[] = "\r\nColl: s (lh M m pl span) h q"
				"  D E F Q\r\n";
#else
static const char welcome_str[] = "\r\nWelcome to Tag Testbed\r\n"
	"Commands:\r\n"
	"\tSet:\ts[lh[ maj[ min[ pl[ span]]]]]\r\n"
	"\tHelp:\th\r\n"
	"\tq(uit)\r\n";
#endif	/* TINY_MEM */

static const char ill_str[] =	"Illegal command (%s)\r\n";

#if TINY_MEM
static const char stats_str1[] = "Stats for hostId: localHost (%lx: %u):\r\n";
static const char stats_str2[] = " In %u, Out %u, Fwd %u\r\n";
static const char stats_str3[] = " Time %lu, Freqs (%u, %u), PLev %x\r\n";
static const char stats_str4[] = " phys: %x, plug: %x, txrx: %x\r\n";
static const char stats_str5[] = " Mem free (%u, %u) min (%u, %u)\r\n";
#else
static const char stats_str[] = "Stats for hostId: localHost (%lx: %u):\r\n"
	" In %u, Out %u, Fwd %u\r\n"
	" Time %lu, Freqs (%u, %u), PLev %x\r\n"
	" phys: %x, plug: %x, txrx: %x\r\n"
	" Mem free (%u, %u) min (%u, %u)\r\n";
#endif 	/* TINY_MEM */

#endif
