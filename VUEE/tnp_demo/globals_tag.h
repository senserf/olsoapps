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
#include "app_tarp_if.h"

#include "attribs_tag.h"

heapmem {80, 20}; // how to find out a good ratio?

static char * ui_ibuf 	= NULL;
static char * ui_obuf 	= NULL;
static char * cmd_line	= NULL;

const   lword	host_id		= 0xBACADEAD;
lword	host_passwd		= 0;

word		app_flags	= 0;

appCountType	app_count	= {0, 0, 0};

// dupa: was 0543, and rx_lev 5
pongParamsType	pong_params = {	1024, //5120, 	// freq_maj
				250, //1024, 	// freq_min
				0x0001, // cc1000 7-8-9 (3 levels) 3-4-5 cc1100
				500, //1024,	// rx_span
				1	// rx_lev: 7 on cc1000, 5 on cc1100
};

#endif

#include "attnames_tag.h"

// These are static const and can thus be shared

#if TINY_MEM
static const char welcome_str[] = "\r\nTag: s (lh M m pl span) h q\r\n";
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
static const char stats_str2[] = " In (%u:%u), Out (%u:%u), Fwd (%u:%u),\r\n";
static const char stats_str3[] = " Time (%lu), Freqs (%u, %u), PLev: %x\r\n";
static const char stats_str4[] = " phys: %x, plug: %x, txrx: %x\r\n";
static const char stats_str5[] = " Mem free (%u, %u) min (%u, %u)\r\n";
#else
static const char stats_str[] = "Stats for hostId: localHost (%lx: %u):\r\n"
	" In (%u:%u), Out (%u:%u), Fwd (%u:%u),\r\n"
	" Time (%lu), Freqs (%u, %u), PLev: %x\r\n"
	" phys: %x, plug: %x, txrx: %x\r\n"
	" Mem free (%u, %u) min (%u, %u)\r\n";
#endif 	/* TINY_MEM */

#endif
