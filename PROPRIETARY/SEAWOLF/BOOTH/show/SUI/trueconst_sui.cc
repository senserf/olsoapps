#include "sysio.h"

trueconst char oss_out_f_str[] = "oss_out failed";

trueconst char d_event[][12] = {
	"_sui", "_lcd", "_kiosk", "_sens",
	"+rxoff", "+tag", "+hunt", "+virt",
	"SPB", "IPV", "Combat", "Avaya",
	"skiing", "golf", "triathlon", "fishing"};

trueconst char d_nbu[][12] = {
	"olsonet", "combat", "latin", "last year",
       	"looks great", "blogs", "career", "night life"};


// output strings same for all devices
trueconst char welcome_str[] = "***Booth 0.2***\r\n"
	"Set / show matching (s, p):\r\n"
	"\tid:\tsi <id>\r\n"
	"\tnickname:\tsn <nickname 7>\r\n"
#if ANDROIDEMO
	"\tdesc:\tsd <description 33>\r\n"
	"\tpriv:\tsp <priv desc 33>\r\n"
	"\tbiz:\tsb <biz desc 33>\r\n"
	"\talrm:\tsa <alrm desc 33>\r\n"
#else
	"\tdesc:\tsd <description 15>\r\n"
	"\tpriv:\tsp <priv desc 15>\r\n"
	"\tbiz:\tsb <biz desc 15>\r\n"
	"\talrm:\tsa <alrm desc 15>\r\n"
#endif
	"\tprofile:\tpp[ |&||]<ABCD hex>\r\n"
	"\texclude:\tpe[ |&||]<ABCD hex>\r\n"
	"\tinclude:\tpi[ |&||]<ABCD hex>\r\n\r\n"
	"Help / bulk shows:\r\n"
	"\tsHow\tsettings:\ths\r\n"
	"\tsHow\tparams\thp\r\n"
	"\tsHow\tevent desc\the [ABCD hex [ABCD hex]]\r\n"
	"\tsHow\tnbuZZ desc\thz [AB hex {AB hex]]\r\n"
	"\tHelp\th\r\n\r\n"
	"Matching actions (U, X, Y, N, T, B, P, A, a, H, S, R, E, K, F, O, L, q, Q):\r\n"
	"\tAUto on / off\tU [1|0]\r\n"
	"\tXmit on / off\tX [1|0]\r\n"
	"\tAccept\t\tY <id>\r\n"
	"\tReject\t\tN <id>\r\n"
	"\tTargeted ping\tT <id>\r\n"
	"\tBusiness\t\tB <id>\r\n"
	"\tPrivate\t\tP <id>\r\n"
	"\tAlarm\t\tA <id> <level>\r\n"
	"\tAlarm beac\ta [1|0] [<id>]\r\n"
  	"\tHunt\t\tH [1|0]\r\n\r\n"
	"\tStore\t\tS <id>\r\n"
	"\tRetrieve\tR [<id>]\r\n"
	"\tErase\t\tE <id>\r\n\r\n"
	"\tKiosk\t\tK <id>\r\n"
	"\tFreqs\t\tF[p|a]<freq>\r\n"
	"\tOSSI\t\tO [0|1|2]\r\n\r\n"
	"\tSave and reset\tq\r\n"
	"\tClear and reset\tQ\r\n"
	"\tList nbuzz/tag/ign/mon\tL[z|t|i|m]\r\n"
	"\tnbuZZ add\tZ+ <id> <what: 0|1> <why: AB hex> <dhook> <memo>\r\n"
	"\tnbuZZ del\tZ- <id>\r\n"
	"\tMonitor add\tM+ <id> <nick>\r\n"
	"\tMonitor del\tM- <id>\r\n";

#ifdef PD_FMT
// output strings for a 'presentation device' (OSS parser)

#define	FMT_MARK	"!PD"
trueconst char hs_str[] = FMT_MARK "01 Nick: %s, Desc: %s,"
				"Biz: %s, Priv: %s, Alrm: %s,"
				"Profile: %x, Exc: %x, Inc: %x\r\n";

trueconst char ill_str[] = FMT_MARK "02 Illegal command (%s)\r\n";

trueconst char bad_str[] =  FMT_MARK 
				"03 Bad or incomplete command (%s)\r\n";

#if ANDROIDEMO
trueconst char stats_str[] = FMT_MARK "STATS,%u,%lu,%u,%u,%u,%u,%u,"
					"%u,%u,%u,%u\r\n";

trueconst char profi_ascii_def[] = FMT_MARK "%s,%u,%u,%s,%u,%lu ago,"
			"state:%s,profile:%x,%s:%s\r\n";
#else
trueconst char stats_str[] = FMT_MARK 
					"04 Stats for %u at %lu babe(%u %u):"
					" Freq audit (%u), events (%u),"
					" PLev (%u),"
					" Mem free (%u, %u) min (%u, %u)\r\n";

trueconst char profi_ascii_def[] = FMT_MARK "05 (%u %u): %s(%u) %lu ago,"
			"%sstate(%s), Profile: %x, Desc(%s%s: %s\r\n";
#endif
trueconst char profi_ascii_raw[] = FMT_MARK 
	"06 nick(%s), id(%u), et(%lu), "
	"lt(%lu), intim(%u), state(%u), profi(%x), desc(%s), info(%x), "
	"pl(%u), rssi (%u)\r\n";

#if ANDROIDEMO
trueconst char alrm_ascii_def[] = FMT_MARK "ALARM,%s,%u,"
	"%x,%u,%u,%d,%s\r\n";
#else
trueconst char alrm_ascii_def[] = FMT_MARK "07 Alrm from %s(%u) "
	"profile(%x) lev(%u) hops(%u) for %d: %s\r\n";
#endif

trueconst char alrm_ascii_raw[] = FMT_MARK "08 nick(%s), id(%u), "
	"profi(%x), lev(%u), hops(%u), for(%u), desc(%s)\r\n";

trueconst char nvm_ascii_def[] = FMT_MARK "09 nvm slot %u: id(%u), "
		"profi(%x), nick(%s), desc(%s), priv(%s), biz(%s)\r\n";

trueconst char nvm_local_ascii_def[] = FMT_MARK "10 nvm slot %u: "
	"id(%u), profi(%x), inc (%x), exc (%x), nick(%s), "
	"desc(%s), priv(%s), biz(%s)\r\n";

trueconst char nb_imp_str[] = FMT_MARK "11 NBuzz imports:\r\n";

trueconst char cur_tag_str[] = FMT_MARK "12 Current tags:\r\n";
 
trueconst char be_ign_str[] = FMT_MARK "13 Being ignored:\r\n";

trueconst char be_mon_str[] = FMT_MARK "14 Being monitored:\r\n";

trueconst char nb_imp_el_str[] = FMT_MARK "15 %c %u dh(%u) %s %s";

trueconst char be_ign_el_str[] = FMT_MARK "16 %u %s\r\n";

trueconst char be_mon_el_str[] = FMT_MARK "17 %u %s\r\n";

trueconst char hz_str[] = FMT_MARK "18 noombuzz %u %s\r\n";

trueconst char he_str[] = FMT_MARK "19 event %u %s\r\n";

#else
// for human eyes (ascii terminal)

trueconst char hs_str[] = 	"Nick: %s, Desc: %s\r\n"
				"Biz: %s, Priv: %s, Alrm: %s\r\n"
				"Profile: %x, Exc: %x, Inc: %x\r\n";

trueconst char ill_str[] =	"Illegal command (%s)\r\n";
trueconst char bad_str[] =   "Bad or incomplete command (%s)\r\n";

trueconst char stats_str[] = "Stats for %u at %lu babe(%u, %u):\r\n"
	" Freq audit (%u) events (%u) PLev (%u)\r\n"
	" Mem free (%u, %u) min (%u, %u)\r\n";

#if ANDROIDEMO
trueconst char profi_ascii_def[] = "\r\n(%s)(%u %u): %s(%u) %lu ago,"
	       "state(%s):\r\n -Profile: %x Desc %s: %s\r\n";
#else
trueconst char profi_ascii_def[] = "\r\n(%u %u): %s(%u) %lu ago,"
		"%sstate(%s):\r\n -Profile: %x Desc(%s%s: %s\r\n";
#endif
trueconst char profi_ascii_raw[] = "nick(%s) id(%u) et(%lu) lt(%lu) "
	"intim(%u) state(%u) profi(%x) desc(%s) info(%x) pl(%u) rssi (%u)\r\n";

trueconst char alrm_ascii_def[] = "Alrm from %s(%u) profile(%x) lev(%u) "
	"hops(%u) for %d:\r\n%s\r\n";

trueconst char alrm_ascii_raw[] = "nick(%s) id(%u) profi(%x) lev(%u) "
	"hops(%u) for(%u) desc(%s)\r\n";

trueconst char nvm_ascii_def[] = "\r\nnvm slot %u: id(%u) profi(%x) "
	"nick(%s)\r\ndesc(%s) priv(%s) biz(%s)\r\n";
	
trueconst char nvm_local_ascii_def[] = 
	"\r\nnvm slot %u: id(%u) profi(%x) "
	"inc (%x) exc (%x) nick(%s)\r\ndesc(%s) priv(%s) biz(%s)\r\n";

trueconst char nb_imp_str[] = "NBuzz imports:\r\n";

trueconst char cur_tag_str[] = "Current tags:\r\n";

trueconst char be_ign_str[] = "Being ignored:\r\n";

trueconst char be_mon_str[] = "Being monitored:\r\n";

trueconst char nb_imp_el_str[] = " %c %u dh(%u) %s %s";

trueconst char be_ign_el_str[] = " %u %s\r\n";

trueconst char be_mon_el_str[] = " %u %s\r\n"; // here and now same as ign

trueconst char hz_str[] = " noombuzz %u\t%s\r\n";

trueconst char he_str[] = " event %u\t%s\r\n";
#endif
