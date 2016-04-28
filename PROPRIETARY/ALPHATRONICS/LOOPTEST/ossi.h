#include "sysio.h"

#define	OSS_PRAXIS_ID		65570
#define	OSS_UART_RATE		115200
#define	OSS_PACKET_LENGTH	56

// =================================================================
// Generated automatically, do not edit (unless you really want to)!
// =================================================================

typedef	struct {
	word size;
	byte content [];
} blob;

typedef	struct {
	byte code, ref;
} oss_hdr_t;

// ==================
// Command structures
// ==================

#define	command_ping_code	1
typedef struct {
	word	pval;
} command_ping_t;

#define	command_ap_code	128
typedef struct {
	byte	worp;
	byte	norp;
	word	worprl;
	word	nodeid;
} command_ap_t;

#define	command_turn_code	2
typedef struct {
	byte	conf;
	byte	mode;
	word	patt;
} command_turn_t;

#define	command_rreg_code	3
typedef struct {
	byte	reg;
} command_rreg_t;

#define	command_wreg_code	4
typedef struct {
	byte	reg;
	byte	val;
} command_wreg_t;

#define	command_wcmd_code	5
typedef struct {
	byte	what;
} command_wcmd_t;

#define	command_radio_code	6
typedef struct {
	word	delay;
} command_radio_t;

// ==================
// Message structures
// ==================

#define	message_status_code	1
typedef struct {
	lword	tstamp;
	lword	status;
} message_status_t;

#define	message_ap_code	128
typedef struct {
	byte	worp;
	byte	norp;
	word	worprl;
	word	nodeid;
} message_ap_t;

#define	message_regval_code	2
typedef struct {
	byte	reg;
	byte	val;
} message_regval_t;


// ===================================
// End of automatically generated code 
// ===================================
