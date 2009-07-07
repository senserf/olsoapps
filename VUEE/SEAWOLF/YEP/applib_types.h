#ifndef	__applib_types_h__
#define	__applib_types_h__

#define BEAC_FREQ	5

// 'what' in handle_nbh()
#define NBH_AUDIT	1
#define NBH_FIND	2
#define NBH_INIT	3

#define AD_GOT		1
#define AD_RPLY		2
#define AD_CANC		3
// hellish kludge - not sure about the advertisers
#define AD_GBN_ID	88
#define EV_ORG_ID	77

// updates c0 (status) or c1 (group) (never both)
#define ULSEL_C0	1
#define ULSEL_C1	2

// handshake status
#define HS_SILE		0
#define HS_BEAC		1
#define HS_ISND		2
#define HS_IRCV		3
#define HS_NOSY		4
#define HS_SYMM		5
#define HS_MATC		6
#define HS_DECL		7
#define HS_IDEC		8

// codes for the status char (menu line[0], after the cursor >)
#define MLI0_INIT	'.'
#define MLI0_INNH	'!'
#define MLI0_GONE	'x'
#define MLI0_NOSY	'0'
#define MLI0_SYMM	'2'
#define MLI0_ISND	'1'
#define MLI0_IRCV	'm'
#define MLI0_MATC	'M'
#define MLI0_DECL	'-'
#define MLI0_IDEC	'~'

// colors for the 'TOP_NH view'
#define COL_NOSY	COLOR_BLACK
#define COL_SYMM	COLOR_CYAN
#define COL_ISND	COLOR_YELLOW
#define COL_IRCV	COLOR_GREEN
#define COL_MATC	COLOR_WHITE
#define COL_DECL	COLOR_RED
#define COL_IDEC	COLOR_PINK

// codes for the group char (menu line[1]), so far +/- with 'ignore'
#define MLI1_YE		'+'
#define MLI1_NO		'-'
#define MLI1_IY		'I'
#define MLI1_IN		'i'

// cxt_flag.top, cxt
#define TOP_HIER	0
#define TOP_DATA	1
#define TOP_AD		2
#define TOP_NH		3
#define TOP_NONH	4

// Menu parameters (sealists doesn't need them any more) ======================

#define	SEA_MENU_CATEV_BG	COLOR_GREEN
#define	SEA_MENU_CATEV_FG	COLOR_BLACK
#define SEA_MENU_CATMY_BG	COLOR_BLUE
#define SEA_MENU_CATMY_FG	COLOR_WHITE

// ============================================================================

typedef struct {
	word	id;
	word	ts;
	word	gr :8; // :2 needed now (byte is loaded from ee though)
	word	st :8; // :4 needed
} nbh_t;

typedef struct {
	word	li 	:8; // :7 enough
	word	num 	:2;
	word	scr	:2;
	word	spare	:4;
	word	cid;	// needed ?
	nbh_t  *mm;
} nbh_menu_t;

typedef struct {
	char	*buf;
	word	 len; // len, rssi could be :8, but need addresses or doublets
	word	 rss;
} rf_rcv_t;

typedef struct {
	word	top : 4;
	word	cxt : 4;
	word	spare : 8;
} cflags_t;	// context (LCD) flags

#endif
