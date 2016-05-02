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
	byte	pval;
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
	byte	what;
} command_turn_t;

#define	command_dump_code	3
typedef struct {
	word	addr;
	word	size;
} command_dump_t;

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

#define	message_dump_code	3
typedef struct {
	blob	bytes;
} message_dump_t;


// ===================================
// End of automatically generated code 
// ===================================
