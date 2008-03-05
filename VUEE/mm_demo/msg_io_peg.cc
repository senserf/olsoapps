/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2008.                   */
/* All rights reserved.                                                 */
/* ==================================================================== */

#ifdef __SMURPH__
#include "globals_peg.h"
#include "threadhdrs_peg.h"
#endif

#include "diag.h"
#include "app_peg.h"
#include "msg_peg.h"

#ifdef	__SMURPH__

#include "node_peg.h"
#include "stdattr.h"

#else	/* PICOS */

#include "net.h"

#endif

#include "attnames_peg.h"

__PUBLF (NodePeg, void, msg_data_out) (nid_t peg, word info) {
	char * buf_out = get_mem (WNONE, sizeof(msgDataType));

	if (buf_out == NULL)
		return;

	in_header(buf_out, msg_type) = msg_data;
	in_header(buf_out, rcv) = peg;
	in_header(buf_out, hco) = 1;
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

__PUBLF (NodePeg, void, msg_data_in) (char * buf) {
	sint tagIndex;

	if ((tagIndex = find_tag (in_header(buf, snd))) < 0) { // not found
		app_diag (D_WARNING, "Spam? %u", in_header(buf, snd));
		return;
	}

	strncpy (tagArray[tagIndex].desc, in_data(buf, desc), PEG_STR_LEN);
	tagArray[tagIndex].info = in_data(buf, info);

	if (tagArray[tagIndex].state == confirmedTag)
		set_tagState(tagIndex, matchedTag, YES);
	
	oss_data_out (tagIndex);
}

__PUBLF (NodePeg, void, msg_profi_out) (nid_t peg) {
	char * buf_out = get_mem (WNONE, sizeof(msgProfiType));

	if (buf_out == NULL)
		return;

	in_header(buf_out, msg_type) = msg_profi;
	in_header(buf_out, rcv) = peg;
	in_header(buf_out, hco) = 1;
	in_profi(buf_out, profi) = profi_att;
	in_profi(buf_out, pl) = host_pl;
	strncpy (in_profi(buf_out, nick), nick_att, NI_LEN);
	send_msg (buf_out, sizeof (msgProfiType));
	ufree (buf_out);
}

__PUBLF (NodePeg, void, msg_alrm_out) (nid_t peg, word level) {
	char * buf_out = get_mem (WNONE, sizeof(msgAlrmType));

	if (buf_out == NULL)
		return;

	in_header(buf_out, msg_type) = msg_alrm;
	in_header(buf_out, rcv) = peg; 
	in_header(buf_out, hco) = 0;
	in_alrm(buf_out, level) = level;
	in_alrm(buf_out, profi) = profi_att;
	strncpy (in_alrm(buf_out, desc), d_alrm, PEG_STR_LEN);
	strncpy (in_alrm(buf_out, nick), nick_att, NI_LEN);
	send_msg (buf_out, sizeof (msgAlrmType));
	ufree (buf_out);
}

__PUBLF (NodePeg, void, msg_profi_in) (char * buf, word rssi) {
	sint tagIndex;

	app_diag (D_DEBUG, "Profi %u", in_header(buf, snd));

	if ((tagIndex = find_ign (in_header(buf, snd))) >= 0) {
		app_diag (D_INFO, "Ignoring %u", in_header(buf, snd));
		return;
	}

	if ((tagIndex = find_tag (in_header(buf, snd))) < 0) { // not found

		tagIndex = insert_tag (buf);
		if (tagIndex >= 0)
			tagArray[tagIndex].rssi = rssi; // rssi not passed in

		return;
	}

	// these may change meaningfully:
	tagArray[tagIndex].rssi = rssi;
	tagArray[tagIndex].pl = in_profi(buf, pl);
	if (in_header(buf, rcv) != 0 && tagArray[tagIndex].intim == 0)
		tagArray[tagIndex].intim = 1;

	switch (tagArray[tagIndex].state) {
		case noTag:
			app_diag (D_SERIOUS, "NoTag error");
			return;

		case newTag:
			oss_profi_out (tagIndex);
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
			app_diag (D_SERIOUS, "Tag state?(%u) Suicide!",
				tagArray[tagIndex].state);
			reset ();
	}
}

__PUBLF (NodePeg, void, msg_alrm_in) (char * buf) {

	oss_alrm_out (buf);
	if (led_state.color != LED_R) {
		leds (led_state.color, LED_OFF);
		led_state.color = LED_R;
		leds (LED_R, led_state.state);
	} else
		led_state.dura = 0;

}
