#ifndef __pg_oss_h
#define	__pg_oss_h

//+++ "oss.cc"

#include "sysio.h"
#include "plug_boss.h"

#define	OSS_PHY		0
#define	OSS_PLUG	0
#define	OSS_UART	UART_A

#define	OSS_SAMPLES	129
#define	OSS_MAXPLEN	(OSS_SAMPLES + 3)

#define	OSS_CMD_NOP	0xFF	// NOP (always ignored, needed for resync)
#define	OSS_CMD_RSC	0xFE	// RESYNC
#define	OSS_CMD_ACK	0xFD
#define	OSS_CMD_NAK	0xFC
#define	OSS_CMD_U_PKT	0x01	// Packet (direction UP)
#define	OSS_CMD_U_SMP	0x02	// Sample (direction UP)
#define	OSS_CMD_D_SMP	0x02	// Sample request (direction DOWN)
#define	OSS_CMD_D_SRG	0x03	// Set register
#define	OSS_CMD_D_GRG	0x04	// Get register
#define	OSS_CMD_U_GRG	0x04	// Get register response
#define	OSS_CMD_D_POL	0x05	// Poll for magic code
#define	OSS_CMD_U_POL	0x05	// Magic code
#define	OSS_CMD_D_SRGB	0x06	// Set register burst
#define	OSS_CMD_D_GRGB	0x07	// Get register burst
#define	OSS_CMD_U_GRGB	0x07

// The response should contain 4 bytes: 0x05 0xCE 0x31 0x7A
#define	OSS_MAG0	0xCE
#define	OSS_MAG1	0x31
#define	OSS_MAG2	0x7A

typedef	void (*cmdfun_t) (word, byte*, word);

void oss_init (cmdfun_t);
void oss_ack (word, byte);
byte *oss_outu (word, sint);
byte *oss_outr (word, sint);
void oss_send (byte*);

#endif
