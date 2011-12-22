#ifndef __str_col_h__
#define __str_col_h__

#include "oss_fmt.h"

#ifdef BOARD_CHRONOS
static trueconst char welcome_str[] = "";
static trueconst char ill_str[] = "";
static trueconst char bad_str[] = "";
static trueconst char stats_str[] = "";
static trueconst char ifla_str[] = "";

#else

static trueconst char welcome_str[] = OPRE_APP_MENU_C 
	"*ILdemo*" OMID_CRB "Collector commands\r\n"
	OPRE_APP_MENU_C 
	"\tSet/ show:\ts [ Maj_freq [ min_freq [ rx_span [ hex:pl_vec"
	" [ hex:c_fl ]]]]]\r\n"
	OPRE_APP_MENU_C
	"\tMaintenance:\tM (*** No collection until F      ***)\r\n"
	OPRE_APP_MENU_C
	"\tFlash erase:\tF (*** clears special conditions  ***)\r\n"
	OPRE_APP_MENU_C
	"\tClean reset:\tQ (*** to factory defaults        ***)\r\n"
	OPRE_APP_MENU_C
	"\tID set / show:\tI[D id]    (*** CAREFUL Host ID   ***)\r\n"
	OPRE_APP_MENU_C
	"\tSave(d) sys:  \tS[A]       (*** Show, SAve iFLASH ***)\r\n"
	OPRE_APP_MENU_C
	"\tQuit (reset)\tq\r\n"
	OPRE_APP_MENU_C
	"\tHelp:\t\th\r\n"
	OPRE_APP_MENU_C
	"\tButton:\t\tB [nr]\r\n";

static trueconst char ill_str[] =	OPRE_APP_ILL
				"Illegal command (%s)\r\n";
static trueconst char bad_str[] =   OPRE_APP_BAD
				"Bad or failed command (%s)[%d]\r\n";

static trueconst char stats_str[] = OPRE_APP_STATS_C
	"Stats for collector (%u: %u):" OMID_CR
	" Mf %u mf %u rx_span %u pl %x c_fl %x" OMID_CR
	" Uptime %lu vtstats %u %u %u %u %u %u\r\n";

static trueconst char ifla_str[] = OPRE_APP_IFLA_C
	"Flash: id %u pl %x c_fl %x Maj_freq %u min_freq %u rx_span %u\r\n";
#endif
#endif
