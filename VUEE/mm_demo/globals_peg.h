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
#include "serf.h"
#include "ser.h"
#include "net.h"
#include "tarp.h"

#include "attribs_peg.h"

heapmem {80, 20}; // how to find out a good ratio?

const   lword	host_id		= 0xBACA000A;
lword	host_passwd		= 0;
word	host_pl			= 1;

char	*ui_ibuf	= NULL,
	*ui_obuf	= NULL,
	*cmd_line	= NULL;

word	app_flags 		= 0;
word	tag_auditFreq 		= 4; // in seconds
word	tag_eventGran		= 4;  // in seconds

tagDataType 	tagArray [LI_MAX];
tagShortType    ignArray [LI_MAX];
tagShortType    monArray [LI_MAX];

ledStateType	led_state	= {0, 0, 0};

word	profi_att	= 0;
word	p_inc 	= 0;
word	p_exc	= 0;
char	nick_att  [NI_LEN]	= {'\0'};
char	desc_att  [PEG_STR_LEN]	= {'\0'};
char	d_biz  [PEG_STR_LEN]	= {'\0'};
char	d_priv  [PEG_STR_LEN]	= {'\0'};
char	d_alrm  [PEG_STR_LEN]	= {'\0'};

#endif

#include "attnames_peg.h"

// These are static const and can thus be shared

static const char welcome_str[] = "Set / show matching (s, p):\r\n"
	"\tnickname:\tsn <nickname 7>\r\n"
	"\tdesc:\t\tsd <description 15>\r\n"
	"\tpriv:\t\tsp <priv desc 15>\r\n"
	"\tbiz:\t\tsb <biz desc 15>\r\n"
	"\talrm:\t\tsa <alrm desc 15>\r\n"
	"\tprofile:\tpp 0xABCD\r\n"
	"\texclude:\tpe 0xABCD\r\n"
	"\tinclude:\tpi 0xABCD\r\n\r\n"
	"Help / bulk shows:\r\n"
	"\tsHow\tsettings:\ths\r\n"
	"\tsHow\tparams\t\thp\r\n"
	"\tHelp\th\r\n\r\n"
	"Matching actions (X, Y, N, T, B, P, A, S, R, L, q, Q):\r\n"
	"\tXmit on / off\tX <1, 0>\r\n"
	"\tAccept\t\tY <id>\r\n"
	"\tReject\t\tN <id>\r\n"
	"\tTargeted ping\tT <id>\r\n"
	"\tBusiness\tB <id>\r\n"
	"\tPrivate\t\tP <id>\r\n"
	"\tAlarm\t\tA <id> <level>\r\n"
	"\tStore\t\tS <id>\r\n"
	"\tRetrieve\tR [<id>]\r\n"
	"\tSave and reset\tq\r\n"
	"\tClear and reset\tQ\r\n"
	"\tList\r\n\t  tag/ign/mon\tL[t|i|m]\r\n"
	"\tMonitor add\tM+ <id> <nick>\r\n"
	"\tMonitor del\tM- <id>\r\n";

static const char hs_str[] = 	"Nick: %s, Desc: %s\r\n"
				"Biz: %s, Priv: %s, Alrm: %s\r\n"
				"Profile: %x, Exc: %x, Inc: %x\r\n";

static const char ill_str[] =	"Illegal command (%s)\r\n";
static const char bad_str[] =   "Bad or incomplete command (%s)\r\n";

static const char stats_str[] = "Stats for hostId - localHost (%lx - %u):\r\n"
	" Freq audit (%u) events (%u) PLev (%u) Time (%lu)\r\n"
	" Mem free (%u, %u) min (%u, %u)\r\n";

static const char profi_ascii_def[] = "\r\n%s: %s (%u) %lu sec. ago,"
		"%sstate(%s):\r\n -Profile: %x Desc(%s): %s\r\n";

static const char profi_ascii_raw[] = "nick(%s) id(%u) et(%lu) lt(%lu) "
	"intim(%u) state(%u) profi(%x) desc(%s) info(%x) pl(%u) rssi (%u)\r\n";

static const char alrm_ascii_def[] = "Alrm from %s(%u) profile(%x) lev(%u) "
	"hops(%u) for %d:\r\n%s\r\n";

static const char alrm_ascii_raw[] = "nick(%s) id(%u) profi(%x) lev(%u) "
	"hops(%u) for(%u) desc(%s)\r\n";
#endif
