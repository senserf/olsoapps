#define	NETWORK_ID	0xA1FA

#define	PKTYPE_WAKE	1
#define	PKTYPE_MOTION	2
#define	PKTYPE_BUTTON	3

#define	PKTYPE_ACK	1

// This is pure payload, excluding STX, ETX, parity, and any DLE's
#define	MAX_RENESAS_MESSAGE_LENGTH	64
