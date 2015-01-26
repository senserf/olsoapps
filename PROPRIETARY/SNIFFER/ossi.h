#include "sysio.h"

#define	OSS_PRAXIS_ID		65543
#define	OSS_UART_RATE		115200
#define	OSS_PACKET_LENGTH	82

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

#define	command_status_code	1
typedef struct {
	byte	what;
} command_status_t;

// ==================
// Message structures
// ==================

#define	message_packet_code	1
typedef struct {
	blob	payload;
} message_packet_t;


// ===================================
// End of automatically generated code 
// ===================================
