/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
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

#define	TRIGGER_VALUE	0xB00

#endif
