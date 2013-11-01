/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2013.                   */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "app_sui_data.h"
#include "msg.h"
#include "diag.h"
#include "form.h"
#include "ser.h"
#include "tarp.h"

#define OSS_ASCII_DEF	0
#define OSS_ASCII_RAW	1
#define oss_fmt	OSS_ASCII_DEF

// =============
// OSS reporting
// =============
fsm oss_out (char*) {
    state OO_RETRY:
		if (app_flags.oss_out == 0) {
			ufree (data);
			finish;
		}
		
		if (data == NULL)
			app_diag_S (oss_out_f_str);
		else
			ser_outb (OO_RETRY, data);

        finish;
}



static const char * stateName (word state) {
	switch ((tagStateType)state) {
		case noTag:
			return "noTag";
		case newTag:
			return "new";
		case reportedTag:
			return "reported";
		case confirmedTag:
			return "confirmed";
		case fadingReportedTag:
			return "fadingReported";
		case fadingConfirmedTag:
			return "fadingConfirmed";
		case goneTag:
			return "gone";
		case sumTag:
			return "sum";
		case fadingMatchedTag:
			return "fadingMatched";
		case matchedTag:
			return "matched";
		default:
			return "unknown?";
	}
}
#if 0
static const char * locatName (word rssi, word pl) { // ignoring pl 
	switch (rssi) {
		case 3:
			return "proxy";
		case 2:
			return "near";
		case 1:
			return "far";
		case 0:
			return "no";
	}
	return "rssi?";
}
#endif
static const char * descName (word info) {
	if (info & INFO_PRIV) return "private";
	if (info & INFO_BIZ) return "business";
	if (info & INFO_DESC) return "intro";
	// noombuzz is independent...
	return "noDesc";
}

static const char * descMark (word info, word list) {

	if (list)
		return "LT";

	if (info & INFO_PRIV) return "PRIV";
	if (info & INFO_BIZ) return "BIZ";
	if (info & INFO_DESC) return "DESC";
	// noombuzz is independent...
	return "HELLO";
}

void oss_over_profi_out (char * buf, word rssi) {
	char * lbuf;

	if (app_flags.oss_out < 2)
		return;
	
	lbuf = form (NULL, profi_ascii_def, "DESC",
		in_profi(buf, pl), rssi, in_profi(buf, nick),
		in_header(buf, snd), 0, "NEW",
		in_profi(buf, profi), "intro", in_profi(buf, desc));
	
	if (runfsm oss_out (lbuf) == 0 ) {
		app_diag_S (oss_out_f_str);
		ufree (lbuf);
    }
}

// arg. list indicates calls from 'lt'; kludgy wrap for 'LT' for Android stuff
void oss_profi_out (word ind, word list) {
	char * lbuf = NULL;

    switch (oss_fmt) {
	case OSS_ASCII_DEF:
		lbuf = form (NULL, profi_ascii_def,
#if ANDROIDEMO
                        descMark(tagArray[ind].info, list),
#endif
			//locatName (tagArray[ind].rssi, tagArray[ind].pl),
			 tagArray[ind].pl, tagArray[ind].rssi,
			tagArray[ind].nick, tagArray[ind].id,
			seconds() - tagArray[ind].evTime,
#if ANDROIDEMO == 0
			tagArray[ind].intim == 0 ? " " : " *intim* ",
#endif
			stateName (tagArray[ind].state),
			tagArray[ind].profi,
			descName (tagArray[ind].info),
#if ANDROIDEMO == 0
			(tagArray[ind].info & INFO_NBUZZ) ? " noombuzz)" : ")",
#endif
			tagArray[ind].desc);
			break;

	case OSS_ASCII_RAW:
		lbuf = form (NULL, profi_ascii_raw,
			tagArray[ind].nick, tagArray[ind].id,
			tagArray[ind].evTime, tagArray[ind].lastTime,
			tagArray[ind].intim, tagArray[ind].state,
			tagArray[ind].profi, tagArray[ind].desc,
			tagArray[ind].info,
			tagArray[ind].pl, tagArray[ind].rssi);
			break;
	default:
		app_diag_S ("**Unsupported fmt %u", oss_fmt);
		return;

    }

    if (runfsm oss_out (lbuf) == 0 ) {
	app_diag_S (oss_out_f_str);
	ufree (lbuf);
    }
}

void oss_data_out (word ind) {
	// for now
	oss_profi_out (ind, 0);
}

void oss_nvm_out (nvmDataType * buf, word slot) {
	char * lbuf = NULL;

	// no matter what format
	if (slot == 0)
		lbuf = form (NULL, nvm_local_ascii_def, slot, buf->id,
			buf->profi, buf->local_inc, buf->local_exc,
			buf->nick, buf->desc, buf->dpriv, buf->dbiz);
	else
		lbuf = form (NULL, nvm_ascii_def, slot, buf->id, buf->profi,
			buf->nick, buf->desc, buf->dpriv, buf->dbiz);

	if (runfsm oss_out (lbuf) == 0 ) {
		app_diag_S (oss_out_f_str);
		ufree (lbuf);
	}
}

void oss_alrm_out (char * buf) {
	char * lbuf = NULL;

    switch (oss_fmt) {
	case OSS_ASCII_DEF:
	    if (buf) {
		lbuf = form (NULL, alrm_ascii_def,
			in_alrm(buf, nick), in_header(buf, snd),
			in_alrm(buf, profi), in_alrm(buf, level),
			in_header(buf, hoc), in_header(buf, rcv),
			in_alrm(buf, desc));
	    } else {
		lbuf = form (NULL, alrm_ascii_def, nick_att, local_host,
				profi_att, 0, 0, local_host, d_alrm);
	    }
		break;

	case OSS_ASCII_RAW:
            if (buf) {
		lbuf = form (NULL, alrm_ascii_raw,
			in_alrm(buf, nick), in_header(buf, snd),
			in_alrm(buf, profi), in_alrm(buf, level),
			in_header(buf, hoc), in_header(buf, rcv),
			in_alrm(buf, desc));
            } else {
                lbuf = form (NULL, alrm_ascii_raw, nick_att, local_host,
                                profi_att, 0, 0, local_host, d_alrm);
            }
		break;
	default:
		app_diag_S ("**Unsupported fmt %u", oss_fmt);
		return;
    }

    if (runfsm oss_out (lbuf) == 0 ) {
	    app_diag_S (oss_out_f_str);
	    ufree (lbuf);
    }
}
