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
#include "storage.h"

heapmem {80, 20}; // how to find out a good ratio?

const   lword	host_id		= 0xBACA0067;
word	host_pl			= 2; // PicOS differs from VUEE

char	*ui_ibuf	= NULL,
	*ui_obuf	= NULL,
	*cmd_line	= NULL;

word	app_flags 		= DEF_APP_FLAGS;
word	tag_auditFreq 		= 4; // in seconds
word	tag_eventGran		= 4;  // in seconds

tagDataType 	tagArray [LI_MAX];
tagShortType    ignArray [LI_MAX];
tagShortType    monArray [LI_MAX];
nbuComType	nbuArray [LI_MAX];

ledStateType	led_state	= {0, 0, 0};

profi_t	profi_att	= 0xFFFF;
profi_t	p_inc 	= 0xFFFF;
profi_t	p_exc	= 0;
char	nick_att  [NI_LEN +1]		= "uninit"; //{'\0'};
char	desc_att  [PEG_STR_LEN +1]	= "uninit desc"; //{'\0'};
char	d_biz  [PEG_STR_LEN +1]		= "uninit biz"; //{'\0'};
char	d_priv  [PEG_STR_LEN +1]	= "uninit priv"; //{'\0'};
char	d_alrm  [PEG_STR_LEN +1]	= {'\0'};

#endif

#include "attnames_peg.h"

// These are static const and can thus be shared

static const char d_event[][12] = {
	"music", "piano", "outdoors", "cinema",
	"bar", "pm trip", "bridge", "squash",
	"workshop B", "workshop A", "thread 2", "thread 1",
	"male", "female", "local", "visiting"};

static const char d_nbu[][12] = {
	"olsonet", "combat", "latin", "last year",
       	"looks great", "blogs", "career", "night life"};

static const char welcome_str[] = "***Seawolfie 0.4***\r\n"
	"Set / show matching (s, p):\r\n"
	"\tnickname:\tsn <nickname 7>\r\n"
	"\tdesc:\t\tsd <description 15>\r\n"
	"\tpriv:\t\tsp <priv desc 15>\r\n"
	"\tbiz:\t\tsb <biz desc 15>\r\n"
	"\talrm:\t\tsa <alrm desc 15>\r\n"
	"\tprofile:\tpp <ABCD hex>\r\n"
	"\texclude:\tpe <ABCD hex>\r\n"
	"\tinclude:\tpi <ABCD hex>\r\n\r\n"
	"Help / bulk shows:\r\n"
	"\tsHow\tsettings:\ths\r\n"
	"\tsHow\tparams\t\thp\r\n"
	"\tsHow\tevent desc\the [ABCD hex [ABCD hex]]\r\n"
	"\tsHow\tnbuZZ desc\thz [AB hex {AB hex]]\r\n"
	"\tHelp\th\r\n\r\n"
	"Matching actions (U, X, Y, N, T, B, P, A, S, R, E, L, q, Q):\r\n"
	"\tAUto on / off\tU [1|0]\r\n"
	"\tXmit on / off\tX [1|0]\r\n"
	"\tAccept\t\tY <id>\r\n"
	"\tReject\t\tN <id>\r\n"
	"\tTargeted ping\tT <id>\r\n"
	"\tBusiness\tB <id>\r\n"
	"\tPrivate\t\tP <id>\r\n"
	"\tAlarm\t\tA <id> <level>\r\n"
	"\tStore\t\tS <id>\r\n"
	"\tRetrieve\tR [<id>]\r\n"
	"\tErase\t\tE <id>\r\n"
	"\tSave and reset\tq\r\n"
	"\tClear and reset\tQ\r\n"
	"\tList\r\n\t  nbuzz/tag/ign/mon\tL[z|t|i|m]\r\n"
	"\tnbuZZ add\tZ+ <id> <what: 0|1> <why: AB hex> <dhook> <memo>\r\n"
	"\tnbuZZ del\tZ- <id>\r\n"
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
		"%sstate(%s):\r\n -Profile: %x Desc(%s%s: %s\r\n";

static const char profi_ascii_raw[] = "nick(%s) id(%u) et(%lu) lt(%lu) "
	"intim(%u) state(%u) profi(%x) desc(%s) info(%x) pl(%u) rssi (%u)\r\n";

static const char alrm_ascii_def[] = "Alrm from %s(%u) profile(%x) lev(%u) "
	"hops(%u) for %d:\r\n%s\r\n";

static const char alrm_ascii_raw[] = "nick(%s) id(%u) profi(%x) lev(%u) "
	"hops(%u) for(%u) desc(%s)\r\n";

static const char nvm_ascii_def[] = "\r\nnvm slot %u: id(%u) profi(%x) "
	"nick(%s)\r\ndesc(%s) priv(%s) biz(%s)\r\n";

static const char nvm_local_ascii_def[] = "\r\nnvm slot %u: id(%u) profi(%x) "
	"inc (%x) exc (%x) nick(%s)\r\ndesc(%s) priv(%s) biz(%s)\r\n";

#endif
