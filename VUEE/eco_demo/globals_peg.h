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
#include "ser.h"
#include "net.h"
#include "tarp.h"

#include "attribs_peg.h"

heapmem {80, 20}; // how to find out a good ratio?

const   lword	host_id		= 0xBACA000A;
word	host_pl			= 7;

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

// These are static const and can thus be shared

static const char welcome_str[] = "***EcoNet***\r\n Aggregator commands:\r\n"
	"\tAgg set / show:\ta id [ audit_freq [ p_lev ]\r\n"
	"\tMaster Time:\tT [ h:m:s ]\r\n"
	"\tDisplay data:\tD [ from [ to [ col_id [ limit ]]]]\r\n"
	"\tEprom erase:\tE (*** deletes aggregated data   ***)\r\n"
	"\tFlash erase:\tF (*** clears special conditions ***)\r\n"
	"\tClean reset:\tQ (*** to factory defaults (E+F) ***)\r\n"
	"\tQuit (reset)\tq\r\n"
	"\tHelp:\t\th\r\n"
	"\tSend master msg\tm [ peg ]\r\n\r\n"
	"\tCol set / show:\tc id agg_id [ Maj_freq [ min_freq [ rx_span "
	"[ hex:pl_vec ]]]]\r\n"
	"\tFind collector:\tf col_id [ agg_id ]]\r\n";

static const char d_str[] =	"Dumped %lu entries for Col %u\r\n";

static const char ill_str[] =	"Illegal command (%s)\r\n";

static const char stats_str[] = "Stats for agg (%lx: %u):\r\n"
	" Audit freq %u, PLev %u, Uptime %lu, Mdelta %ld, Master %u\r\n"
	" Stored entries %lu, Mem free %u min %u\r\n";

static const char statsCol_str[] = "Stats for coll (%lx: %u) via agg %u:\r\n"
	" Maj_freq %u min_freq %u rx_span %u pl_vec %x\r\n"
	" Uptime %lu Stored reads %lu Mem free %u min %u\r\n";

static const char bad_str[] =   "Bad or incomplete command (%s)\r\n";

static const char clock_str[] = "At %s %u.%u:%u:%u uptime %lu\r\n";

#endif
