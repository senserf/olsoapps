#ifndef __pg_oss_h
#define	__pg_oss_h

//+++ "oss.cc"

#include "sysio.h"
#include "plug_boss.h"

#define	OSS_PHY		0
#define	OSS_PLUG	0
#define	OSS_UART	UART_A

#define	OSS_MAXSEGMENTS	24

#define	OSS_MAXPLEN	((OSS_MAXSEGMENTS * 4) + 10)

#define	OSS_CMD_NOP	0xFF	// NOP (always ignored, needed for resync)
#define	OSS_CMD_RSC	0xFE	// RESYNC
#define	OSS_CMD_ACK	0xFD
#define	OSS_CMD_NAK	0xFC
#define	OSS_CMD_D_RUN	0x01	// Load profile and GO (direction UP)
#define	OSS_CMD_D_ABT	0x02	// Abort
#define	OSS_CMD_D_SET	0x03	// Set oven
#define	OSS_CMD_D_EMU   0x04	// Emulation mode on/off
#define	OSS_CMD_U_REP	0x01	// Report
#define	OSS_CMD_U_HBE	0x7C	// Heart beat (+ MAGIC)

// ============================================================================
// Packet formats:
//
//	RUN:  0x01 NS-1 SP-2 IN-2 DI-2 DU-2 TM-2 ... DU-2 TM-2 (NS times)
//	ABT:  0x02 0x02
//	SET:  0x03 0x03 OV-2
//	EMU:  0x04 0x00 [or 0x01]
//	REP:  0x01 0xEM SE-2 TM-2 OV-2 SG-2 TA-2 (EM = emulation flag)
//	HBE:  0x7C 0xEM 0x31 0x7A TM-2 OV-2	 (EM = emulation flag)
// ============================================================================

typedef	void (*cmdfun_t) (word, byte*, word);

void oss_init (cmdfun_t);
void oss_ack (word, byte);
byte *oss_outu (word, sint);
byte *oss_outr (word, sint);
void oss_send (byte*);

#endif
