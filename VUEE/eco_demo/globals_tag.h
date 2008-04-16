#ifndef __globals_tag_h__
#define __globals_tag_h__

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

lword	 ref_time = 0;
mclock_t ref_clock = {0};

char * ui_ibuf 	= NULL;
char * ui_obuf 	= NULL;
char * cmd_line	= NULL;

extern const lword host_id;
//+++ "hostid.c"

word		app_flags	= 0;

pongParamsType	pong_params = {	900, //30	// freq_maj in sec, max 63K
				5,  	// freq_min in sec. max 63
				0x7777, // levels / retries
				1, 	// rx_span in msec (max 63K) 1: ON
				0,	// rx_lev: select if needed, 0: all
				0	// pload_lev: same
};

sensDataType	sens_data;
long		lh_time;
sensEEDumpType	*sens_dump = NULL;
#endif

#include "attnames_tag.h"

// These are static const and can thus be shared

static const char welcome_str[] = "***EcoNet***\r\nCollector commands\r\n"
	"\tSet/ show:\ts [ Maj_freq [ min_freq [ rx_span [ hex:pl_vec ]]]]\r\n"
	"\tDisplay data:\tD [ from [ to [ status [ limit ]]]]\r\n"
	"\tEprom erase:\tE (*** deletes all collected data ***)\r\n"
	"\tFlash erase:\tF (*** clears special conditions  ***)\r\n"
	"\tClean reset:\tQ (*** to factory defaults (E+F)  ***)\r\n"
	"\tQuit (reset)\tq\r\n"
	"\tHelp:\t\th\r\n";

static const char ill_str[] =	"Illegal command (%s)\r\n";
static const char bad_str[] =   "Bad or incomplete command (%s)\r\n";

static const char stats_str[] = "Stats for collector (%lx: %u):\r\n"
	" Maj_freq %u min_freq %u rx_span %u pl_vec %x\r\n"
	" Uptime %lu Stored reads %lu Mem free %u min %u\r\n";

#endif
