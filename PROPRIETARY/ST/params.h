/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2012.                          */
/* All rights reserved.                                                 */
/* ==================================================================== */
#ifndef __params_h
#define __params_h

#define	NETWORK_ID	0x5AB0
#define	PACKET_LENGTH	14
#define	MAGIC		0xABC0

#define	REPEAT_COUNT	4	// Retransmissions
#define	REPEAT_DELAY	256
#define	TRANSMIT_MARGIN	1024	// Stay ON after last transmission

#define	CYCLE_INTERVAL	900	// Report interval (seconds)

#define	REPORT_TIMEOUT	(4 * CYCLE_INTERVAL)

#define	MONITOR_INTERVAL	4096

#define	TRIGGER_VALUE	1000

#endif
