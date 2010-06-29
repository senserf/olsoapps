/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2008.                          */
/* All rights reserved.                                                 */
/* ==================================================================== */
#ifndef __oss_fmt_h
#define __oss_fmt_h

/*********************************************************************
  These are definitions for output via OSSI formatting.

FMT_MAIN: Default format for our standard OSSI:
	nnnn <payload> \r\n

FMT_TERM: ASCII terminal assumed, multi-line output:
	<payload line 1> \r\n
	<payload line 2> \r\n

In options.sys, #define OSS_FMT <?> does the (static) switch.
After that, all work is done by the preprocessor and compiler.

Corresponding format strings aew in globals_peg/tag.cc, or within the calls
to app_diag (OPRE_DIAG) or diag (mostly OPRE_APP_ACK).

The strings are freely modifiable if needed, but that should be done within
compile time optons.

Mnemonics:

OPRE_		OSSI Prefix ...

OPRE_SYS	... System: SYSERRs and banners (not converted yet)
OPRE_DIAG	,,, Diagnostics: Should be passed to the operator as is.

OMID_		OSSI Mid-output ...

OMID_CR		... CR: removes CR from multi-line output
OMID_CRB	... CR to blank: replaces CR with blank
OMID_CRC	... CR to comma: replaces CR with comma and blank

OPRE_APP_	OSSI Prefix Application -specific ...

OPRE_APP_MENU_A Menu (Help) on the Aggregator (welcome_str)
OPRE_APP_MENU_C             on the Collector (welcome_str)
OPRE_APP_ILL	Illegal command (ill_str)
OPRE_APP_BAD	bad or failed command (bad_str)
OPRE_APP_ACK	Acknowledgement of requested actions with no other output,
		e.g. 'm'. Mostly via diag()

OPRE_APP_REP	Report with sensor readings (rep_str)
OPRE_APP_REP_SUM Summary report (repSum_str)
OPRE_APP_REP_NO  No collectors found (repNo_str)

OPRE_APP_STATS_A  Result from 'a' command: stats from Agg (stats_str)
OPRE_APP_STATS_CA Result from 'c' command: stats from Col via Agg (statsCol_str)
OPRE_APP_STATS_C  Result from 's' command: stats from Col (stats_str)

OPRE_APP_DUMP_A	Single output line after 'D' on the Agg (dump_str)
OPRE_APP_DEND_A Closing line after 'D' on the Agg (dumpend_str)
OPRE_APP_DUMP_C Single output line after 'D' on the Col (dump_str)
OPRE_APP_DEND_C Closing line after 'D' on the Col (dumpend_str)
OPRE_APP_T	After the 'T' command
OPRE_APP_P	After the 'P' command

OPRE_APP_IFLA_A	Result from 'S' command: Agg EFLASH sys content (ifla_str)
OPRE_APP_IFLA_C	Result from 'S' command: Col EFLASH sys content (ifla_str)

*********************************************************************/

#define FMT_MAIN	1
#define FMT_TERM	2

#ifndef OSS_FMT
#define OSS_FMT		FMT_MAIN
#endif

#if OSS_FMT == FMT_MAIN

#define OPRE_SYS		"9000 "
#define OPRE_DIAG		"8000 "

#define OMID_CR			""
#define OMID_CRB		" "
#define OMID_CRC		", "

#define OPRE_APP_MENU_A		"1001 "
#define OPRE_APP_MENU_C 	"2001 "
#define OPRE_APP_ILL		"0001 "
#define OPRE_APP_BAD		"0002 "
#define OPRE_APP_ACK		"0003 "

#define OPRE_APP_REP		"1002 " OMID_CR
#define OPRE_APP_REP_SUM	"1003 "
#define OPRE_APP_REP_NO		"1004 "
#define OPRE_APP_REP_GONE	"1014 " OMID_CR

#define OPRE_APP_STATS_C	"2005 "
#define OPRE_APP_STATS_CA	"1006 "
#define OPRE_APP_STATS_A	"1005 "

#define OPRE_APP_DUMP_A		"1007 "
#define OPRE_APP_DUMP_C		"2007 "
#define OPRE_APP_DEND_A		"1008 "
#define OPRE_APP_DEND_C		"2008 "
#define OPRE_APP_T		"1009 "

#define OPRE_APP_IFLA_A		"1010 "
#define OPRE_APP_IFLA_C		"2010 "

#define OPRE_APP_DMARK_A	"1011 "
#define OPRE_APP_DMARK_C	"2011 "

#define OPRE_APP_PLOT		"1012 "
#define OPRE_APP_SYNC		"1013 "
#else

#if OSS_FMT == FMT_TERM

#define OPRE_SYS		""
#define OPRE_DIAG		""

#define OMID_CR			"\r\n"
#define OMID_CRB		OMID_CR
#define OMID_CRC		OMID_CR


#define OPRE_APP_MENU_A		""
#define OPRE_APP_MENU_C 	""
#define OPRE_APP_ILL		""
#define OPRE_APP_BAD		""
#define OPRE_APP_ACK		""

#define OPRE_APP_REP		"" OMID_CR
#define OPRE_APP_REP_SUM	""
#define OPRE_APP_REP_NO		""
#define OPRE_APP_REP_GONE	"" OMID_CR

#define OPRE_APP_STATS_C	""
#define OPRE_APP_STATS_CA	""
#define OPRE_APP_STATS_A	""

#define OPRE_APP_DUMP_A		""
#define OPRE_APP_DUMP_C		""
#define OPRE_APP_DEND_A		""
#define OPRE_APP_DEND_C		""
#define OPRE_APP_T		""

#define OPRE_APP_IFLA_A		""
#define OPRE_APP_IFLA_C		""

#define OPRE_APP_DMARK_A        ""
#define OPRE_APP_DMARK_C        ""

#define OPRE_APP_PLOT           "" 
#define OPRE_APP_SYNC           "" 

#else

#error "undefined OSSI"

#endif  // FMT_TERM
#endif  // FMT_MAIN

#endif
