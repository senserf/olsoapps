#ifndef __oss_cus_str_h
#define __oss_cus_str_h

#include "oss_fmt.h"

// These are static const and can thus be shared
static trueconst char ee_str[] =
	OPRE_APP_MENU_C "EE from %lu to %lu size %u\r\n";

static trueconst char welcome_str[] = OPRE_APP_MENU_A 
	"*EcoNet* 1.3" OMID_CRB "Custodian commands:\r\n"
	OPRE_APP_MENU_A 
	"\tAgg set / show:\ta id [ audit_freq [ p_lev [ hex:a_fl ]]]\r\n"
	OPRE_APP_MENU_A 
	"\tDisplay data:\tD [ from [ to [ col_id [ limit ]]]]\r\n"
	OPRE_APP_MENU_A 
	"\tMaintenance:\tM (*** No collection until F     ***)\r\n"
	OPRE_APP_MENU_A
	"\tEprom erase:\tE (*** deletes aggregated data   ***)\r\n"
	OPRE_APP_MENU_A 
	"\tFlash erase:\tF (*** clears special conditions ***)\r\n"
	OPRE_APP_MENU_A 
	"\tClean reset:\tQ (*** to factory defaults (E+F) ***)\r\n"
	OPRE_APP_MENU_A
	"\tID set / show:\tI[D id]   (*** CAREFUL Host ID   ***)\r\n"
	OPRE_APP_MENU_A
	"\tID master set:\tIM id     (*** CAREFUL Master ID ***)\r\n"
	OPRE_APP_MENU_A 
	"\tSave(d) sys:  \tS[A]      (*** Show, SAve iFLASH ***)\r\n"
	OPRE_APP_MENU_A 
	"\tQuit (reset)\tq\r\n"
	OPRE_APP_MENU_A 
	"\tHelp:\t\th\r\n"
	OPRE_APP_MENU_A 
	"\tCol set / show:\tc id agg_id [ Maj_freq [ min_freq [ rx_span "
	"[ hex:pl [ hex:c_fl]]]]]\r\n"
	OPRE_APP_MENU_A 
	"\tFind collector:\tf col_id [ agg_id ]]\r\n"
	OPRE_APP_MENU_A
	"\tRpc:\t\tr host_id <cmd>\r\n"
	OPRE_APP_MENU_A
	"\tSat mode:\ts!!!|+++|---\r\n"
	OPRE_APP_MENU_A
	"\tSat test:\ts[ ]<text>\r\n";

static trueconst char ill_str[] =	OPRE_APP_ILL 
				"Illegal command (%s)\r\n";

static trueconst char not_in_maint_str[] =
	OPRE_APP_ILL "Not in Maintenance\r\n";

static trueconst char only_master_str[] = OPRE_APP_ILL "Only at the Master\r\n";

static trueconst char stats_str[] = OPRE_APP_STATS_A 
	"Stats for agg (%lx: %u):" OMID_CR
	" Audit %u PLev %u a_fl %x Uptime %lu Mts %ld Master %u" OMID_CR
	" Stored %lu Mem free %u min %u mode %u inp %u\r\n";

static trueconst char statsCol_str[] = OPRE_APP_STATS_CA
	"Stats for coll (%lx: %u) via %u:" OMID_CR
	" Maj_freq %u min_freq %u rx_span %u pl %x c_fl %x" OMID_CR
	" Uptime %lu Stored reads %lu Mem free %u min %u\r\n";

static trueconst char ifla_str[] = OPRE_APP_IFLA_A
	"Flash: id %u pl %u a_fl %x au_fr %u master %u sync_fr %u mode %u\r\n";

static trueconst char bad_str[] = 	OPRE_APP_BAD 
				"Bad or failed command (%s)[%d]\r\n";

static trueconst char clock_str[] = OPRE_APP_T 
				"At %u-%u-%u %u:%u:%u uptime %lu\r\n";

static trueconst char rep_str[] = OPRE_APP_REP OMID_CR
	"  Agg %u slot %lu: %u-%u-%u %u:%u:%u" OMID_CR
	"  Col %u slot %lu: %u-%u-%u %u:%u:%u%s" OMID_CR
	"  " SENS0_DESC "%d " SENS1_DESC "%d " SENS2_DESC "%d "
	SENS3_DESC "%d " SENS4_DESC "%d " SENS5_DESC "%d\r\n";

static trueconst char repSum_str[] = OPRE_APP_REP_SUM
	"Agg %u handles %u collectors\r\n";

static trueconst char repNo_str[] = OPRE_APP_REP_NO
	"No Col %u at Agg %u\r\n";

static trueconst char dump_str[] = OPRE_APP_DUMP_A OMID_CR
	"Col %u slot %lu (A: %lu) %u-%u-%u %u:%u:%u (A %u-%u-%u %u:%u:%u)"
	OMID_CR " " SENS0_DESC "%d " SENS1_DESC "%d "
	SENS2_DESC "%d " SENS3_DESC "%d " SENS4_DESC "%d "
	SENS5_DESC "%d\r\n";

static trueconst char dumpend_str[] = OPRE_APP_DEND_A
	"Did coll %u slots %lu -> %lu upto %u #%lu\r\n";

#endif
