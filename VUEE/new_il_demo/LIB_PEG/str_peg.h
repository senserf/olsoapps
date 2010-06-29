#ifndef __str_peg_h__
#define __str_peg_h__

#include "oss_fmt.h"

// PiComp
//
// trueconst means a true constant, i.e., something that can be kept in code
// flash. For VUEE, this will be stored outside of node in a single copy for
// all nodes.
static trueconst char ee_str[] = OPRE_APP_MENU_A \
	"EE from %lu to %lu size %u\r\n";

static trueconst char welcome_str[] = OPRE_APP_MENU_A 
	"*ILdemo*" OMID_CRB "Aggregator commands:\r\n"
	OPRE_APP_MENU_A 
	"\tAgg set / show:\ta id [ audit_freq [ p_lev [ hex:a_fl ]]]\r\n"
	OPRE_APP_MENU_A 
	"\tMaster Time:\tT [ y-m-d h:m:s ]\r\n"
	OPRE_APP_MENU_A 
	"\tDisp data:\tD [ from [ to [ col_id [ limit ]]]]\r\n"
	OPRE_APP_MENU_A 
	"\tMaint:\tM (* No collection until F *)\r\n"
	OPRE_APP_MENU_A
	"\tEprom erase:\tE (* deletes agg data *)\r\n"
	OPRE_APP_MENU_A 
	"\tFlash erase:\tF (* clears special conditions *)\r\n"
	OPRE_APP_MENU_A 
	"\tClean reset:\tQ (* to factory defaults (E+F) *)\r\n"
	OPRE_APP_MENU_A
	"\tID set / show:\tI[D id]  (* CAREFUL Host ID *)\r\n"
	OPRE_APP_MENU_A
	"\tID master set:\tIM id    (* CAREFUL Master ID *)\r\n"
	OPRE_APP_MENU_A 
	"\tSave(d) sys:  \tS[A]     (* Show, SAve iFLASH *)\r\n"
	OPRE_APP_MENU_A
	"\tSync coll:    \tY [freq] (* Sync at freq *)\r\n"
	OPRE_APP_MENU_A 
	"\tQuit (reset)\tq\r\n"
	OPRE_APP_MENU_A 
	"\tHelp:\t\th\r\n"
	OPRE_APP_MENU_A 
	"\tSend master msg\tm [ peg ]" OMID_CR "\r\n"
	OPRE_APP_MENU_A 
	"\tCol set / show:\tc id agg_id [ Maj_freq [ min_freq [ rx_span "
	"[ hex:pl [ hex:c_fl]]]]]\r\n"
	OPRE_APP_MENU_A 
	"\tFind collector:\tf col_id [ agg_id ]]\r\n"
	OPRE_APP_MENU_A
	"\tPlot Id:\tP [id]\r\n";

static trueconst char ill_str[] =	OPRE_APP_ILL 
				"Illegal command (%s)\r\n";

static trueconst char not_in_maint_str[] = OPRE_APP_ILL "Not in Maint\r\n";

static trueconst char only_master_str[] = OPRE_APP_ILL "Only at the Master\r\n";

static trueconst char stats_str[] = OPRE_APP_STATS_A 
	"Stats for agg (%lx: %u):" OMID_CR
	" Audit %u PLev %u a_fl %x Uptime %lu Mts %ld Master %u" OMID_CR
	" mstats %u %u %u %u inp %u\r\n";

static trueconst char statsCol_str[] = OPRE_APP_STATS_CA
	"Stats for coll (%lx: %u)" OMID_CR
	" Mf %u mf %u rx_span %u pl %x c_fl %x" OMID_CR
	" Uptime %lu mstats %u %u %u %u\r\n";

static trueconst char ifla_str[] = OPRE_APP_IFLA_A
	"Flash: id %u pl %u a_fl %x au_fr %u master %u sync_fr %u mode %u\r\n";

static trueconst char bad_str[] = 	OPRE_APP_BAD 
				"Bad or failed command (%s)[%d]\r\n";

static trueconst char clock_str[] = OPRE_APP_T 
				"At %u-%u-%u %u:%u:%u uptime %lu\r\n";

static trueconst char rep_str[] =
	"%s  Agg %u : %u-%u-%u %u:%u:%u" OMID_CR
	"  Col %u : %u-%u-%u %u:%u:%u%s" OMID_CR
	"  " SENS0_DESC "%u " SENS1_DESC "%u " SENS2_DESC "%u "
	SENS3_DESC "%u " SENS4_DESC "%u " SENS5_DESC "%u %u %lu\r\n";

static trueconst char repSum_str[] = OPRE_APP_REP_SUM
	"Agg %u handles %u collectors\r\n";

static trueconst char repNo_str[] = OPRE_APP_REP_NO
	"No Col %u at Agg %u\r\n";

static trueconst char dump_str[] = OPRE_APP_DUMP_A OMID_CR
	"Col %u slot %lu (A: %lu) %u-%u-%u %u:%u:%u (A %u-%u-%u %u:%u:%u)"
	OMID_CR " " SENS0_DESC "%u " SENS1_DESC "%u "
	SENS2_DESC "%u " SENS3_DESC "%u " SENS4_DESC "%u "
	SENS5_DESC "%u\r\n";

static trueconst char dumpmark_str[] = OPRE_APP_DMARK_A "%s %lu %u-%u-%u %u:%u:%u "
	"%u %u %u\r\n";

static trueconst char dumpend_str[] = OPRE_APP_DEND_A
	"Did coll %u slots %lu -> %lu upto %u #%lu\r\n";

static trueconst char plot_str[] = OPRE_APP_PLOT "Plot %u\r\n";

static trueconst char sync_str[] = OPRE_APP_SYNC "Synced to %u\r\n";

static trueconst char impl_date_str[] = OPRE_APP_ACK "Implicit T 9-1-1 0:0:1";

#endif
