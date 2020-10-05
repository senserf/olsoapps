/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/

// ============================================================================
// Extract from params.h of RFTEST
// ============================================================================

#ifndef	POFF_NID

#define	POFF_NID	0	// Offsets in words
#define	POFF_DRI	1	// Driver field
#define	POFF_RCV	2	// Recipient (zero for a measurement packet)
#define	POFF_SND	3	// Sender
#define	POFF_SER	4	// Serial number
#define	POFF_FLG	5	// Flag field
#define	POFF_SEN	7	// First sensor location (measurement packet)
#define	POFF_CMD	6	// Command code (command packet)
#define	POFF_ACT	6	// Node ID to collect ACKs (measurement packet)

#endif
