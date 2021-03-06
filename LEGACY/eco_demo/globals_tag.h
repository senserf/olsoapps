/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
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
#include "storage.h"

#include "attribs_tag.h"

heapmem {80, 20}; // how to find out a good ratio?

lword	 ref_ts   = 0;
lint	 ref_date = 0;

char * ui_ibuf 	= NULL;
char * ui_obuf 	= NULL;
char * cmd_line	= NULL;

extern const lword host_id;
//+++ "hostid.c"

word app_flags	= DEF_APP_FLAGS;
word plot_id	= 0;

pongParamsType	pong_params = {	60,	// freq_maj in sec, max 63K
				5,  	// freq_min in sec. max 63
				0x7777, // levels / retries
				2048, 	// rx_span in msec (max 63K) 1: ON
				0,	// rx_lev: select if needed, 0: all
				0	// pload_lev: same
};

sensDataType	sens_data;
lint		lh_time;
sensEEDumpType	*sens_dump = NULL;
#endif

#include "attnames_tag.h"
#include "oss_fmt.h"

// These are static const and can thus be shared
static const char ee_str[] = OPRE_APP_MENU_C "EE from %lu to %lu size %u\r\n";

static const char welcome_str[] = OPRE_APP_MENU_C 
	"*EcoNet* 1.3" OMID_CRB "Collector commands\r\n"
	OPRE_APP_MENU_C 
	"\tSet/ show:\ts [ Maj_freq [ min_freq [ rx_span [ hex:pl_vec"
	" [ hex:c_fl ]]]]]\r\n"
	OPRE_APP_MENU_C
	"\tDisplay data:\tD [ from [ to [ status [ limit ]]]]\r\n"
	OPRE_APP_MENU_C
	"\tMaintenance:\tM (*** No collection until F      ***)\r\n"
	OPRE_APP_MENU_C
	"\tEprom erase:\tE (*** deletes all collected data ***)\r\n"
	OPRE_APP_MENU_C
	"\tFlash erase:\tF (*** clears special conditions  ***)\r\n"
	OPRE_APP_MENU_C
	"\tClean reset:\tQ (*** to factory defaults (E+F)  ***)\r\n"
	OPRE_APP_MENU_C
	"\tID set / show:\tI[D id]    (*** CAREFUL Host ID   ***)\r\n"
	OPRE_APP_MENU_C
	"\tSave(d) sys:  \tS[A]       (*** Show, SAve iFLASH ***)\r\n"
	OPRE_APP_MENU_C
	"\tQuit (reset)\tq\r\n"
	OPRE_APP_MENU_C
	"\tHelp:\t\th\r\n";

static const char ill_str[] =	OPRE_APP_ILL
				"Illegal command (%s)\r\n";
static const char bad_str[] =   OPRE_APP_BAD
				"Bad or failed command (%s)[%d]\r\n";

static const char stats_str[] = OPRE_APP_STATS_C
	"Stats for collector (%lx: %u):" OMID_CR
	" Maj_freq %u min_freq %u rx_span %u pl %x c_fl %x" OMID_CR
	" Uptime %lu Stored reads %lu Mem free %u min %u\r\n";

static const char ifla_str[] = OPRE_APP_IFLA_C
	"Flash: id %u pl %x c_fl %x Maj_freq %u min_freq %u rx_span %u\r\n";

static const char dump_str[] = OPRE_APP_DUMP_C OMID_CR
	"%s slot %lu %u-%u-%u %u:%u:%u: " SENS0_DESC "%d "
	SENS1_DESC "%d " SENS2_DESC "%d " SENS3_DESC "%d "
       	SENS4_DESC "%d " SENS5_DESC "%d\r\n";

static const char dumpmark_str[] = OPRE_APP_DMARK_C "%s %lu %u-%u-%u %u:%u:%u "
	"%u %u %u\r\n";

static const char dumpend_str[] = OPRE_APP_DEND_C
	"Collector %u direct dump: slots %lu -> %lu status %s upto %u #%lu\r\n";

#endif
