#ifndef	__pg_taglib_h
#define __pg_taglib_h

#include "sysio.h"

//+++ "taglib.cc"

#define	THE_LED			3

#define	FAILURE_BLINK_TIME	4	// seconds (target = 30)
#define	FAILURE_BLINK_ON	205
#define	FAILURE_BLINK_OFF	FAILURE_BLINK_ON
#define	WACK_ON_TIME		2048
#define	MAX_TRIES		3
#define	ACK_WAIT_TIME		1024

#define	HOST_ID			((word) host_id)

void receiver_on ();
void receiver_off ();
void start_up ();
address send_button (byte, byte);

#endif
