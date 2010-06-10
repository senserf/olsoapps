#ifndef __str_tag_h__
#define __str_tag_h__

#include "oss_fmt.h"

static trueconst char ee_str[] = OPRE_APP_MENU_C "EE from %lu to %lu size %u\r\n";

static trueconst char welcome_str[] = OPRE_APP_MENU_C 
	"*ILdemo*" OMID_CRB "Collector commands\r\n"
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

static trueconst char ill_str[] =	OPRE_APP_ILL
				"Illegal command (%s)\r\n";
static trueconst char bad_str[] =   OPRE_APP_BAD
				"Bad or failed command (%s)[%d]\r\n";

static trueconst char stats_str[] = OPRE_APP_STATS_C
	"Stats for collector (%lx: %u):" OMID_CR
	" Mf %u mf %u rx_span %u pl %x c_fl %x" OMID_CR
	" Uptime %lu mstats %u %u %u %u\r\n";

static trueconst char ifla_str[] = OPRE_APP_IFLA_C
	"Flash: id %u pl %x c_fl %x Maj_freq %u min_freq %u rx_span %u\r\n";

static trueconst char dump_str[] = OPRE_APP_DUMP_C OMID_CR
	"%s slot %lu %u-%u-%u %u:%u:%u: " SENS0_DESC "%d "
	SENS1_DESC "%d " SENS2_DESC "%d " SENS3_DESC "%d "
       	SENS4_DESC "%d " SENS5_DESC "%d\r\n";

static trueconst char dumpmark_str[] = OPRE_APP_DMARK_C "%s %lu %u-%u-%u %u:%u:%u "
	"%u %u %u\r\n";

static trueconst char dumpend_str[] = OPRE_APP_DEND_C
	"Collector %u direct dump: slots %lu -> %lu status %s upto %u #%lu\r\n";

#endif
