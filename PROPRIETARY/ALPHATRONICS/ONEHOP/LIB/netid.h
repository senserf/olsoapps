/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#define	NETWORK_ID	0xA1FA

#define	PKTYPE_WAKE	1
#define	PKTYPE_MOTION	2
#define	PKTYPE_BUTTON	3

#define	PKTYPE_ACK	1
#define	PKTYPE_RENESAS	127

// This is pure payload, excluding STX, ETX, parity, and any DLE's
#define	MAX_RENESAS_MESSAGE_LENGTH	64

#define	MASTER_NODE_ID	1

#define	RNTRIES		4

#define	HOSTID		((word)host_id)
