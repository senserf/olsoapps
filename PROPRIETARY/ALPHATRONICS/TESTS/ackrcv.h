#ifndef __pg_ackrcv_h
#define	__pg_ackrcv_h

Boolean	RS = NO, WACK = NO;

word	event_count;

static void receiver_on () {

	if (!RS) {
		tcv_control (sfd, PHYSOPT_ON, NULL);
		RS = YES;
	}
}

static void receiver_off () {

	if (RS) {
		tcv_control (sfd, PHYSOPT_OFF, NULL);
		RS = NO;
	}
}

fsm receiver {

	state WPACKET:

		address pkt;
		word len, cmd;

		pkt = tcv_rnp (WPACKET, sfd);
		len = tcv_left (pkt);

		if (len < 8 || (pkt [1] != 0 && pkt [1] != HOST_ID))
			goto Drop;

		if (pkt [2] == PKTYPE_ACK && WACK && pkt [3] == event_count) {
			WACK = NO;
			trigger (&WACK);
		}
Drop:
		tcv_endp (pkt);
		proceed WPACKET;
}
			
#endif
