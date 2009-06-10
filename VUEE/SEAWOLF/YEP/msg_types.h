#ifndef	__msg_types_h__
#define	__msg_types_h__

#include "tarp.h"

#define MSG_BEAC	1
#define MSG_ACT		2
#define MSG_AD		3
#define MSG_ADACK	4

typedef struct msgBeacStruct {
	headerType	header;
} msgBeacType;

typedef struct msgActStruct {
	headerType      header;
	word		act :8;
	word		ref :8; // likely not needed: hshake's prev. act
} msgActType;

typedef struct msgAdStruct {
	headerType      header;
	word		ref :8; // may be many ads
	word		lev :3;
	word		len :5;
} msgAdType;

typedef struct msgAdAckStruct {
	headerType      header;
	word            ref :8;
	word		ack :8;
} msgAdAckType;

#endif

