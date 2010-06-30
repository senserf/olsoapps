/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2010.			*/
/* All rights reserved.							*/
/* ==================================================================== */

#include "diag.h"
#include "app_peg.h"
#include "msg_peg.h"
#include "net.h"
#include "tarp.h"
#include "app_peg_data.h"

/*
 * "Virtual" stuff needed by NET & TARP =======================================
 */
idiosyncratic int tr_offset (headerType *h) {
	// Unused ??
	return 0;
}

idiosyncratic Boolean msg_isBind (msg_t m) {
	return NO;
}

idiosyncratic Boolean msg_isTrace (msg_t m) {
	return NO;
}

idiosyncratic Boolean msg_isMaster (msg_t m) {
	return NO;
}

idiosyncratic Boolean msg_isNew (msg_t m) {
	return NO;
}

idiosyncratic Boolean msg_isClear (byte o) {
	return YES;
}

idiosyncratic void set_master_chg () {
	app_flags |= 2;
}

// ============================================================================

#ifdef __SMURPH__
// VUEE uses standard string funcs... FIXME
void strncpy (char *d, const char *s, sint n) {
	while (n-- && (*s != '\0'))
		*d++ = *s++;
	*d = '\0';
}
#endif

char *get_mem (word state, int len) {
	char * buf = (char *)umalloc (len);
	word mmin, mem;
	mem = memfree(0, &mmin);
#if 0
	if (mem < 1000) {
		diag ("FIXME %u %u %u %u", mem, mmin, len, state);
		show_stuff (2);
	}
#endif
	if (buf == NULL) {
		app_diag (D_WARNING, "No mem %d", len);
		if (state != WNONE) {
			umwait (state);
			release;
		}
	}
	return buf;
}

void init () {

	nvm_t nvm;

	if (ee_read (NVM_OSET, (byte *)&nvm, sizeof (nvm_t))) {
		app_diag (D_SERIOUS, "Can't read eeprom");
		halt();
	};

	if (nvm.mh == 0xFFFF) { // all defaults
		master_host = DEF_MHOST;
		touts.iv.b.sil = DEF_I_SIL;
		touts.iv.b.warm = DEF_I_WARM;
		touts.iv.b.cyc = DEF_I_CYC;
		sattr[0].b.pl = DEF_PL_FR;
		sattr[0].b.ra = DEF_RA_FR;
		sattr[0].b.ch = DEF_CH_FR;
		sattr[1].b.pl = DEF_PL_TO;
		sattr[1].b.ra = DEF_RA_TO;
		sattr[1].b.ch = DEF_CH_TO;
	} else {
		master_host = nvm.mh;
		local_host = nvm.lh;
		touts.iv.w = nvm.iv.w;
		sattr[0].w = nvm.attr[0].w;
		sattr[1].w = nvm.attr[1].w;
	}

	sattr[3].w = sattr[2].w = sattr[0].w;
	touts.ts = 0;
	touts.mc = 0;
	cters[0] = cters[1] = cters[2] = 0;

	set_state (ST_INIT);
	net_opt (PHYSOPT_RXON, NULL);
	net_opt (PHYSOPT_TXOFF, NULL);
	set_rf (1);
}

void set_state (word s) {

	if (sstate == s)
		return;

	switch (s) {
		case ST_INIT:
			touts.mc = 0;
			leds (LED_G, LED_OFF);
			leds (LED_B, LED_OFF);
			leds (LED_R, LED_ON);
			break;

		case ST_WARM:
			touts.mc = local_host == master_host ?
				touts.mc = touts.iv.b.warm : 0;
			leds (LED_R, LED_OFF);
			leds (LED_B, LED_OFF);
			leds (LED_G, LED_BLINK);
			break;

		case ST_CYC:
			touts.mc = 0;
			cters[0] = cters[1] = cters[2] = 0;
			leds (LED_R, LED_OFF);
			leds (LED_B, LED_OFF);
			leds (LED_G, LED_ON);
			break;

		case ST_COOL:
			touts.mc = local_host == master_host ?
				touts.iv.b.sil : 0;
			leds (LED_R, LED_OFF);
			leds (LED_G, LED_OFF);
			leds (LED_B, LED_ON);
			break;

		default:
			app_diag (D_SERIOUS, "Bad state to set %d", s);
			return;
	}
	sstate = s;
	touts.ts = seconds();
}

void set_rf (word mode) {

	word s = net_opt (PHYSOPT_STATUS, NULL);
	word w;
	net_opt (PHYSOPT_RXOFF, NULL);
	net_opt (PHYSOPT_TXOFF, NULL);
	// FIXME is this good?:
	net_qera (TCV_DSP_XMT);
	net_qera (TCV_DSP_RCV);

	w = mode ? sattr[3].b.pl : DEF_PL_FR;
	net_opt (PHYSOPT_SETPOWER, &w);

	w = mode ? sattr[3].b.ch : DEF_CH_FR;
	net_opt (PHYSOPT_SETCHANNEL, &w);
	app_diag (D_DEBUG, "chan %d", w);

// PHYSOPT_SETRATE resets RF; do it last
	w = mode ? sattr[3].b.ra : DEF_RA_FR;
	net_opt (PHYSOPT_SETRATE, &w);
	app_diag (D_DEBUG, "rate %d", w);
	// net_opt (PHYSOPT_RESET, NULL);

	if (s & 2)
		net_opt (PHYSOPT_TXON, NULL);
	if (s & 1)
		net_opt (PHYSOPT_RXON, NULL);
}

void send_msg (char * buf, int size) {

	// this shouldn't be... WARNING to see why it is needed...
	if (in_header(buf, rcv) == local_host) {
		app_diag (D_WARNING, "Dropped msg(%u) to lh",
			in_header(buf, msg_type));
		return;
	}

	switch (net_tx (WNONE, buf, size, 0)) {
	  case 0: // success
		app_diag (D_DEBUG, "Sent %u to %u",
			in_header(buf, msg_type),
			in_header(buf, rcv));
		return;

	  case 1: // xmt off
		app_diag (D_WARNING, "xmt off for %u to %u",
				in_header(buf, msg_type),
				in_header(buf, rcv));
		return;

	  default:
		app_diag (D_SERIOUS, "Tx %u failed",
			in_header(buf, msg_type));
	}
 }

int check_msg_size (char * buf, word size, word repLevel) {

	word expSize;
	
	// for some msgTypes, it'll be less trivial
	switch (in_header(buf, msg_type)) {

		case msg_master:
			if ((expSize = sizeof(msgMasterType)) == size)
				return 0;
			break;

		case msg_ping:
			if ((expSize = sizeof(msgPingType)) == size)
				return 0;
			break;

		case msg_stats:
			if ((expSize = sizeof(msgStatsType)) == size)
				return 0;
			break;

		case msg_sil:
			if ((expSize = sizeof(msgSilType)) == size)
				return 0;
			break;

		case msg_cmd:
			if ((expSize = sizeof(msgCmdType)) == size)
				return 0;
			break;

		default:
			app_diag (repLevel, "Can't check size of %u (%d)",
				in_header(buf, msg_type), size);
			return 0;
	}
	
	// 4N padding might have happened
	if (size > expSize && size >> 2 == expSize >> 2) {
		app_diag (repLevel, "Inefficient size of %u (%d)",
				in_header(buf, msg_type), size);
		return 0;
	}

	app_diag (repLevel, "Size error for %u: %d of %d",
			in_header(buf, msg_type), size, expSize);
	return (size - expSize);
}
