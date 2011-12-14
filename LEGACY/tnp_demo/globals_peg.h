#ifndef __globals_peg_h__
#define __globals_peg_h__

#define TINY_MEM 1

// Make the generic headers look as close as possible in both cases. This
// involves adding a few (dummy) links to VUEE/PICOS (done).

#include "app_peg.h"
#include "msg_peg.h"
#include "diag.h"
#include "tarp.h"
#include "form.h"
#include "ser.h"
#include "net.h"

heapmem {80, 20}; // how to find out a good ratio?

#ifdef	__SMURPH__

#include "node_peg.h"
#include "stdattr.h"

#else	/* PICOS */

#include "attribs_peg.h"

const   lword	host_id		= 0xBACA000A;
lword	host_passwd		= 0;
word	host_pl			= 1;

char	*ui_ibuf	= NULL,
	*ui_obuf	= NULL,
	*cmd_line	= NULL;

word	app_flags 		= 0;
lint   	master_delta 		= 0;
word	tag_auditFreq 		= 2048; //10240; 	// in bin msec
word	tag_eventGran		= 2; //10; 		// in seconds

// if we can get away with it, it's better to have it in IRAM (?)
tagDataType tagArray [tag_lim];

wroomType msg4tag 		= {NULL, 0};
wroomType msg4ward 		= {NULL, 0};

appCountType app_count 		= {0, 0, 0};

#endif

#include "attnames_peg.h"

// These are static const and can thus be shared

#if TINY_MEM
static const char welcome_str[] = 
			"\r\nPeg: s (lh mid pl) m f g t r h q\r\n";
#else
static const char welcome_str[] = "Commands:\r\n"
	"\tSet:\ts [ lh [ mid [ pl ]]]\r\n"
	"\tMaster:\tm [ peg ]\r\n"
	"\tFind:\tf [ tag [ peg [ rss [ pLev ]]]]\r\n"
	"\tFwd get:g [ tag [ peg ]]\r\n"
	"\tFwd set:t [ tag [ peg ]]\r\n"
	"\tFwd peg:r [ peg [ npeg [ pwr [ str ]]]]\r\n"
	"\tHelp:\th\r\n"
	"\tq\r\n";
#endif

static const char o_str[] =	"phys: %x, plug: %x, txrx: %x\r\n";

static const char ill_str[] =	"Illegal command (%s)\r\n";

#if TINY_MEM
static const char stats_str1[] = "Stats for hostId - localHost (%lx - %u):\r\n";
static const char stats_str2[] = " In (%u:%u), Out (%u:%u), Fwd (%u:%u),\r\n";
static const char stats_str3[] = " Freq audit (%u) events (%u),\r\n";
static const char stats_str4[] = 
	" PLev (%u), Time (%lu), Delta (%ld) to Master (%u),\r\n";
static const char stats_str5[] = " phys: %x, plug: %x, txrx: %x\r\n";
static const char stats_str6[] = " Mem free (%u, %u) min (%u, %u)\r\n";
#else
static const char stats_str[] = "Stats for hostId - localHost (%lx - %u):\r\n"
	" In (%u:%u), Out (%u:%u), Fwd (%u:%u),\r\n"
	" Freq audit (%u) events (%u),\r\n"
	" PLev (%u), Time (%lu), Delta (%ld) to Master (%u),\r\n"
	" phys: %x, plug: %x, txrx: %x\r\n"
	" Mem free (%u, %u) min (%u, %u)\r\n";
#endif /* TINY_MEM */

#endif
