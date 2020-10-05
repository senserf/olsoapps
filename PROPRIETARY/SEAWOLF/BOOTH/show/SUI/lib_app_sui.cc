/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/

#include "diag.h"
#include "app_sui_data.h"
#include "msg.h"
#include "oss_sui.h"

#include "net.h"
#include "tarp.h"


void nbuVec (char * s, byte b) {
	int i = 7;
	do {
		s[7 - i] = (b & (1 << i)) ? '|' : '.';

	} while (--i >= 0);
}

sint find_tag (word tag) {
	word i = 0;
	while (i < LI_MAX) {
		if (tagArray[i].id == tag) {
			return i;
		}
		i++;
	}
	return -1; // found no tag
}

// do NOT combine: real should be quite different
sint find_ign (word tag) {
	word i = 0;
	while (i < LI_MAX) {
		if (ignArray[i].id == tag) {
			return i;
		}
		i++;
	}
	return -1;
}

sint find_nbu (word tag) {
	word i = 0;
	while (i < LI_MAX) {
		if (nbuArray[i].id == tag) {
			return i;
		}
		i++;
	}
	return -1;
}

sint find_mon (word tag) {
	word i = 0;
	while (i < LI_MAX) {
		if (monArray[i].id == tag) {
			return i;
		}
		i++;
	}
	return -1;
}

void init_tag (word i) {
	tagArray[i].id = 0;
	tagArray[i].state = noTag;
	tagArray[i].rssi = 0;
	tagArray[i].pl = 0;
	tagArray[i].intim = 0;
	tagArray[i].info = 0;
	tagArray[i].profi = 0;
	tagArray[i].evTime = 0;
	tagArray[i].lastTime = 0;
	tagArray[i].nick[0] = '\0';
	tagArray[i].desc[0] = '\0';
}

void init_ign (word i) {
	ignArray[i].id = 0;
	ignArray[i].nick[0] = '\0';
}

void init_nbu (word i) {
	nbuArray[i].id = 0;
	nbuArray[i].what = 0;
	nbuArray[i].dhook = 0;
	nbuArray[i].vect = 0;
	nbuArray[i].memo[0] = '\0';
}

void init_mon (word i) {
	monArray[i].id = 0;
	monArray[i].nick[0] = '\0';
}

void init_tags () {
	word i = LI_MAX;
	while (i-- > 0) {
		init_tag (i);
		init_ign (i);
		init_mon (i);
	}
}

void set_tagState (word i, tagStateType state, Boolean updEvTime) {
	tagArray[i].state = state;
	tagArray[i].lastTime = seconds();
	if (updEvTime)
		tagArray[i].evTime = tagArray[i].lastTime;
	app_diag_D ("set_tagState in %u to %u", i, state);
}

static void fill_in (char * buf, word i) {
	tagArray[i].id = in_header(buf, snd);
	tagArray[i].profi = in_profi(buf, profi);
	set_tagState (i, newTag, YES);
	strncpy (tagArray[i].nick, in_profi(buf, nick), NI_LEN);
	strncpy (tagArray[i].desc, in_profi(buf, desc), 
			PEG_STR_LEN);
	tagArray[i].info |= INFO_DESC;
	tagArray[i].pl = in_profi(buf, pl);
	tagArray[i].intim = in_header(buf, rcv) != 0 ? 1 : 0;
}

sint insert_tag (char * buf) {
	sint i = 0;
	sint virti = LI_MAX;
	Boolean dongle = !(in_profi(buf, profi) & PROF_VIRT);

	while (i < LI_MAX) {
		if (tagArray[i].id == 0) {
			fill_in (buf, i);
			app_diag_D ("Inserted tag %u at %u",
					in_header(buf, snd), i);
			return i;
		}
		if (dongle && virti == LI_MAX &&
				(tagArray[i].profi & PROF_VIRT))
			virti = i;
			
		i++;
	}
	
	if (virti != LI_MAX) { // we'll force out virtual for real dongle
		set_tagState (virti, forcedOutTag, NO);
		oss_profi_out (virti, 0);
		init_tag (virti);
		fill_in (buf, virti);
		app_diag_D ("ForcedIn tag %u at %u",
			in_header(buf, snd), virti);
		return virti;
	}
	
	// caller will take care of it
	// app_diag_S ("Failed tag (%u) insert", in_header(buf, snd));
	return -1;
}

sint insert_nbu (word id, word w, word v, word h, char *s) {
	int i = 0;

	while (i < LI_MAX) {
		if (nbuArray[i].id == 0) {
			nbuArray[i].id = id;
			nbuArray[i].what = w;
			nbuArray[i].dhook = h;
			nbuArray[i].vect = v;
			strncpy (nbuArray[i].memo, s, NI_LEN);
			 app_diag_D ("Inserted nbu %u at %u",
					 id, i);
			 return i;
		}
		i++;
	}
	app_diag_S ("Failed nbu (%u) insert", id);
	return -1;
}

sint insert_ign (word id, char * s) {
	int i = 0;

	while (i < LI_MAX) {
		if (ignArray[i].id == 0) {
			ignArray[i].id = id;
			strncpy (ignArray[i].nick, s, NI_LEN);
			app_diag_D ("Inserted ign %u at %u",
					id, i);
			return i;
		}
		i++;
	}
	app_diag_S ("Failed ign (%u) insert", id);
	return -1;
}

sint insert_mon (word id, char * s) {
	int i = 0; 

	while (i < LI_MAX) {
		if (monArray[i].id == 0) {
			monArray[i].id = id;
			strncpy (monArray[i].nick, s, NI_LEN);
			app_diag_D ("Inserted mon %u at %u",
					id, i);
			return i;
		}
		i++;
	}
	app_diag_S ("Failed mon (%u) insert", id);
	return -1;
}

fsm mbeacon;

void check_tag (word i) {

	if (i >= LI_MAX) {
		app_diag_F ("tagAr bound %u", i);
		return;
	}

	if (tagArray[i].id == 0)
		return;

	if (tagArray[i].state == matchedTag) {
		if (led_state.color == LED_R) {
			if (led_state.dura != 0) {
				led_state.dura = 0;
				if (running (mbeacon))
					led_state.color = LED_G;
				else
					led_state.color = LED_B;
				leds (LED_R, LED_OFF);
				leds (led_state.color, LED_BLINK);
				led_state.state = LED_BLINK;
			} else { // keep red for 1 audit period
				led_state.dura ++;
				if (led_state.state != LED_BLINK) {
					leds (LED_R, LED_BLINK);
					led_state.state = LED_BLINK;
				}
			}
		} else { // not red
			if (led_state.state != LED_BLINK) {
				leds (led_state.color, LED_BLINK);
				led_state.state = LED_BLINK;
			}
		}
	}

	if (seconds() - tagArray[i].lastTime < tag_eventGran)
		return;	

	switch (tagArray[i].state) {
		case newTag:
			app_diag_D ("Delete %u", tagArray[i].id);
			init_tag (i);
			break;

		case goneTag:
		case forcedOutTag:
			app_diag_S ("Gone?? %u", tagArray[i].id);
			init_tag (i);
			//oss_profi_out (i, 0);
			break;

		case reportedTag:
			set_tagState (i, fadingReportedTag, NO);
			break;

		case confirmedTag:
			set_tagState (i, fadingConfirmedTag, NO);
			break;

		case matchedTag:
			set_tagState (i, fadingMatchedTag, NO);
			break;

		case fadingReportedTag:
		case fadingConfirmedTag:
		case fadingMatchedTag:
			set_tagState (i, goneTag, NO /*YES not here*/);
			oss_profi_out (i, 0);
			init_tag (i);
			break;

		default:
			app_diag_S ("noTag? %u in %u",
				tagArray[i].id, tagArray[i].state);
	}
}

sint check_msg_size (char * buf, word size) {
	word expSize;
	
	// for some msgTypes, it'll be less trivial
	switch (in_header(buf, msg_type)) {

		case msg_profi:
			if ((expSize = sizeof(msgProfiType)) == size)
				return 0;
			break;

		case msg_data:
			if ((expSize = sizeof(msgDataType)) == size)
				return 0;
			break;

		case msg_alrm:
			if ((expSize = sizeof(msgAlrmType)) == size)
				return 0;
			break;

		default:
			app_diag_S ("Can't check size of %u (%d)",
				in_header(buf, msg_type), size);
			return 0;
	}
	
	// 4N padding might have happened
	if (size > expSize && size >> 2 == expSize >> 2) {
		app_diag_W ("Inefficient size of %u (%d)",
				in_header(buf, msg_type), size);
		return 0;
	}

	app_diag_S ("Size error for %u: %d of %d",
			in_header(buf, msg_type), size, expSize);
	return (size - expSize);
}

