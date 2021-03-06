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


void msg_data_out (nid_t peg, word info) {
	char * buf_out = get_mem (WNONE, sizeof(msgDataType));

	if (buf_out == NULL)
		return;

	in_header(buf_out, msg_type) = msg_data;
	in_header(buf_out, rcv) = peg;
	in_header(buf_out, hco) = 1;
	in_header(buf_out, prox) = 1;
	in_data(buf_out, info) = info;
	if (info & INFO_PRIV)
		strncpy (in_data(buf_out, desc), d_priv, PEG_STR_LEN);
	else if (info & INFO_BIZ)
		strncpy (in_data(buf_out, desc), d_biz, PEG_STR_LEN);
	else
		strncpy (in_data(buf_out, desc), desc_att, PEG_STR_LEN);
	send_msg (buf_out, sizeof (msgDataType));
	ufree (buf_out);
}

void msg_data_in (char * buf) {
	sint tagIndex;

	if ((tagIndex = find_tag (in_header(buf, snd))) < 0) { // not found
		app_diag_W ("Spam? %u", in_header(buf, snd));
		return;
	}

	strncpy (tagArray[tagIndex].desc, in_data(buf, desc), PEG_STR_LEN);
	tagArray[tagIndex].info |= in_data(buf, info);

	if (tagArray[tagIndex].state == confirmedTag)
		set_tagState(tagIndex, matchedTag, YES);

	if (app_flags.autoack) {
		if (tagArray[tagIndex].state == reportedTag)
			set_tagState (tagIndex, confirmedTag, NO);

		if (!(in_data(buf, info) & INFO_ACK))
			msg_data_out (in_header(buf, snd),
					in_data(buf, info) | INFO_ACK);
	}

	oss_data_out (tagIndex);
}

void msg_profi_out (nid_t peg) {
	char * buf_out = get_mem (WNONE, sizeof(msgProfiType));

	if (buf_out == NULL)
		return;

	in_header(buf_out, msg_type) = msg_profi;
	in_header(buf_out, rcv) = peg;
	in_header(buf_out, hco) = 1;
	in_header(buf_out, prox) = 1;
	in_profi(buf_out, profi) = profi_att;
	in_profi(buf_out, pl) = host_pl;
	strncpy (in_profi(buf_out, nick), nick_att, NI_LEN);
	strncpy (in_profi(buf_out, desc), desc_att, PEG_STR_LEN);
	send_msg (buf_out, sizeof (msgProfiType));
	ufree (buf_out);
}

void msg_alrm_out (nid_t peg, word level, char * desc) {
	char * buf_out = get_mem (WNONE, sizeof(msgAlrmType));

	if (buf_out == NULL)
		return;

	in_header(buf_out, msg_type) = msg_alrm;
	in_header(buf_out, rcv) = peg; 
	in_header(buf_out, hco) = (level == 9 ? 0 : 1);
	in_header(buf_out, prox) = (level == 9 ? 0 : 1);
	in_alrm(buf_out, level) = level;
	in_alrm(buf_out, profi) = profi_att;
	strncpy (in_alrm(buf_out, nick), nick_att, NI_LEN);

	if (desc)
		strncpy (in_alrm(buf_out, desc), desc, PEG_STR_LEN);
	else
		strncpy (in_alrm(buf_out, desc), d_alrm, PEG_STR_LEN);

#ifdef __SMURPH__
	// overwrite for hybrid demos
	if ( desc && strncmp (desc, "!Y!", 3) == 0) {
		if (local_host & 1) // odd
			strncpy (in_alrm(buf_out, desc), "!N!", 3);
		else
			strncpy (in_alrm(buf_out, desc), "!OK!", 4);
	}
#endif

	send_msg (buf_out, sizeof (msgAlrmType));
	ufree (buf_out);
}

void msg_profi_in (char * buf, word rssi) {
	sint tagIndex, nbu;
	app_diag_D ("Profi %u", in_header(buf, snd));

	if ((nbu = find_nbu (in_header(buf, snd))) >= 0)
		nbu = nbuArray[nbu].what;

	if (nbu == 0 || find_ign (in_header(buf, snd)) >= 0) {
		app_diag_I ("Ign %u nbu(%d)", in_header(buf, snd), nbu);
		return;
	}

	if (nbu < 0 && ((in_profi(buf, profi) & p_exc) /* ||
			!(in_profi(buf, profi) & p_inc)*/)) {
		app_diag_I ("Rejected %u (%x)", in_header(buf, snd),
				in_profi(buf, profi));
		return;
	}

	if ((tagIndex = find_tag (in_header(buf, snd))) < 0) { // not found

		tagIndex = insert_tag (buf);
		if (tagIndex >= 0) {
			tagArray[tagIndex].rssi = rssi; // rssi not passed in
			if (nbu == 1)
				tagArray[tagIndex].info |= INFO_NBUZZ;
		} else {
			oss_over_profi_out (buf, rssi);
		}

		return;
	}

	// let's refresh all data (may be not that good with larger pings)
	tagArray[tagIndex].rssi = rssi;
	if (nbu == 1)
		tagArray[tagIndex].info |= INFO_NBUZZ;
	tagArray[tagIndex].pl = in_profi(buf, pl);
	if (in_header(buf, rcv) != 0 && tagArray[tagIndex].intim == 0)
		tagArray[tagIndex].intim = 1;
	strncpy (tagArray[tagIndex].nick, in_profi(buf, nick), NI_LEN);

	// this desc in beacon is really across...
	if (tagArray[tagIndex].info <= INFO_DESC) {
		strncpy (tagArray[tagIndex].desc, in_profi(buf, desc), 
				PEG_STR_LEN);
		tagArray[tagIndex].info |= INFO_DESC;
	}

	switch (tagArray[tagIndex].state) {
		case noTag:
			app_diag_S ("NoTag error");
			return;

		case newTag:
			oss_profi_out (tagIndex, 0);
			set_tagState (tagIndex, reportedTag, NO);
			break;

		case reportedTag:
		case confirmedTag:
		case matchedTag:
			tagArray[tagIndex].lastTime = seconds();
			break;

		case fadingReportedTag:
			set_tagState(tagIndex, reportedTag, NO);
			break;

		case fadingConfirmedTag:
			set_tagState(tagIndex, confirmedTag, NO);
			break;

		case fadingMatchedTag:
			set_tagState(tagIndex, matchedTag, NO);
			break;

		case goneTag:
			set_tagState(tagIndex, newTag, YES);
			break;

		default:
			app_diag_S ("Tag state?(%u) Suicide!",
				tagArray[tagIndex].state);
			reset ();
	}
}

void msg_alrm_in (char * buf) {
	app_diag_D ("Alrm %u", in_header(buf, snd));

	if (find_ign (in_header(buf, snd)) >= 0) {
		app_diag_I ("Ign alrm %u", in_header(buf, snd));
		return;
	}

	// not monitored and bad or no match
	if (find_mon (in_header(buf, snd)) < 0 &&
		((in_alrm(buf, profi) & p_exc) /*||
		!(in_alrm(buf, profi) & p_inc)*/)) {
		app_diag_I ("Rejected alrm %u (%x)", in_header(buf, snd),
				in_alrm(buf, profi));
		return;
	}

	if (in_alrm(buf, level) == 7 
#ifdef __SMURPH__
			|| strncmp (in_alrm(buf, desc), "!Y!", 3) == 0
#endif
		)
		msg_alrm_out (in_header(buf, snd), 77, in_alrm(buf, desc));

	oss_alrm_out (buf);
	if (led_state.color != LED_R) {
		leds (led_state.color, LED_OFF);
		led_state.color = LED_R;
		leds (LED_R, led_state.state);
	} else
		led_state.dura = 0;

}
