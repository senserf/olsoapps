#ifndef __globals_peg_h__
#define __globals_peg_h__

#include "app_peg.h"
#include "msg_peg.h"
#include "diag.h"
#include "oss_fmt.h"

#ifdef	__SMURPH__

#include "node_peg.h"
#include "stdattr.h"

#else	/* PICOS */

#include "form.h"
#include "ser.h"
#include "net.h"
#include "tarp.h"

#include "attribs_peg.h"

heapmem {80, 20}; // how to find out a good ratio?

extern const lword host_id;
//+++ "hostid.c"

word	host_pl		= 7;

char	*ui_ibuf	= NULL,
	*ui_obuf	= NULL,
	*cmd_line	= NULL;

lword	master_delta		= 0;
word	app_flags 		= 0;
word	tag_auditFreq 		= 13;	// in seconds
mclock_t master_clock		= {0};

// if we can get away with it, it's better to have it in IRAM (?)
tagDataType tagArray [tag_lim];

wroomType msg4tag 		= {NULL, 0};
wroomType msg4ward 		= {NULL, 0};

aggDataType	agg_data;
msgPongAckType	pong_ack	= {{msg_pongAck}};

aggEEDumpType	*agg_dump = NULL;



#endif

#include "attnames_peg.h"
#include "oss_fmt.h"

// These are static const and can thus be shared

static const char welcome_str[] = OPRE_APP_MENU_A 
	"***EcoNet***" OMID_CRB "Aggregator commands:\r\n"
	OPRE_APP_MENU_A 
	"\tAgg set / show:\ta id [ audit_freq [ p_lev ]\r\n"
	OPRE_APP_MENU_A 
	"\tMaster Time:\tT [ h:m:s ]\r\n"
	OPRE_APP_MENU_A 
	"\tDisplay data:\tD [ from [ to [ col_id [ limit ]]]]\r\n"
	OPRE_APP_MENU_A 
	"\tEprom erase:\tE (*** deletes aggregated data   ***)\r\n"
	OPRE_APP_MENU_A 
	"\tFlash erase:\tF (*** clears special conditions ***)\r\n"
	OPRE_APP_MENU_A 
	"\tClean reset:\tQ (*** to factory defaults (E+F) ***)\r\n"
	OPRE_APP_MENU_A 
	"\tQuit (reset)\tq\r\n"
	OPRE_APP_MENU_A 
	"\tHelp:\t\th\r\n"
	OPRE_APP_MENU_A 
	"\tSend master msg\tm [ peg ]" OMID_CR "\r\n"
	OPRE_APP_MENU_A 
	"\tCol set / show:\tc id agg_id [ Maj_freq [ min_freq [ rx_span "
	"[ hex:pl ]]]]\r\n"
	OPRE_APP_MENU_A 
	"\tFind collector:\tf col_id [ agg_id ]]\r\n";

static const char ill_str[] =	OPRE_APP_ILL 
				"Illegal command (%s)\r\n";

static const char stats_str[] = OPRE_APP_STATS_A 
	"Stats for agg (%lx: %u):" OMID_CR
	" Audit freq %u PLev %u Uptime %lu Mdelta %ld Master %u" OMID_CR
	" Stored entries %lu Mem free %u min %u\r\n";

static const char statsCol_str[] = OPRE_APP_STATS_CA
	"Stats for coll (%lx: %u) via %u:" OMID_CR
	" Maj_freq %u min_freq %u rx_span %u pl %u" OMID_CR
	" Uptime %lu Stored reads %lu Mem free %u min %u\r\n";

static const char bad_str[] = 	OPRE_APP_BAD 
				"Bad or incomplete command (%s)\r\n";

static const char clock_str[] = OPRE_APP_T 
				"At %s %u.%u:%u:%u uptime %lu\r\n";

static const char rep_str[] = OPRE_APP_REP OMID_CR
	"  Agg %u slot: %lu, %s: %u.%u:%u:%u" OMID_CR
	"  Col %u slot: %lu, %s: %u.%u:%u:%u%s" OMID_CR
	"  PAR: %d, T: %d, H: %d, PD: %d, T2: %d\r\n";

static const char repSum_str[] = OPRE_APP_REP_SUM
	"Agg %u handles %u collectors\r\n";

static const char repNo_str[] = OPRE_APP_REP_NO
	"No Col %u at Agg %u\r\n";

static const char dump_str[] = OPRE_APP_DUMP_A OMID_CR
	"Col %u slot %lu (A: %lu) %s: %u.%u:%u:%u (A %s: %u.%u:%u:%u)"
	OMID_CR " PAR: %d, T: %d, H: %d, PD: %d, T2: %d\r\n";

static const char dumpend_str[] = OPRE_APP_DEND_A
	"Did coll %u slots %lu -> %lu upto %u #%lu\r\n";

#endif
