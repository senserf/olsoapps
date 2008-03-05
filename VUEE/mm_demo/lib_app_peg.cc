/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2006.			*/
/* All rights reserved.							*/
/* ==================================================================== */

#include "diag.h"
#include "app_peg.h"
#include "msg_peg.h"

#ifdef	__SMURPH__

#include "node_peg.h"
#include "stdattr.h"

#else	/* PICOS */

#include "net.h"
#include "tarp.h"

#endif	/* SMURPH or PICOS */

#include "threadhdrs_peg.h"
#include "attnames_peg.h"

/*
 * "Virtual" stuff needed by NET & TARP =======================================
 */
__PUBLF (NodePeg, int, tr_offset) (headerType *h) {
	// Unused ??
	return 0;
}

__PUBLF (NodePeg, Boolean, msg_isBind) (msg_t m) {
	return NO;
}

__PUBLF (NodePeg, Boolean, msg_isTrace) (msg_t m) {
	return NO;
}

__PUBLF (NodePeg, Boolean, msg_isMaster) (msg_t m) {
	return NO;
}

__PUBLF (NodePeg, Boolean, msg_isNew) (msg_t m) {
	return NO;
}

__PUBLF (NodePeg, Boolean, msg_isClear) (byte o) {
	return YES;
}

__PUBLF (NodePeg, void, set_master_chg) () {
	app_flags |= 2;
}

// ============================================================================

__PUBLF (NodePeg, int, find_tag) (word tag) {
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
__PUBLF (NodePeg, int, find_ign) (word tag) {
	word i = 0;
	while (i < LI_MAX) {
		if (ignArray[i].id == tag) {
			return i;
		}
		i++;
	}
	return -1;
}

__PUBLF (NodePeg, int, find_mon) (word tag) {
	word i = 0;
	while (i < LI_MAX) {
		if (ignArray[i].id == tag) {
			return i;
		}
		i++;
	}
	return -1;
}

#ifdef __SMURPH__
// VUEE uses standard string funcs... FIXME
__PUBLF (NodePeg, void, strncpy) (char *d, const char *s, sint n) {
	while (n-- && (*s != '\0'))
		*d++ = *s++;
	*d = '\0';
}
#endif

__PUBLF (NodePeg, char*, get_mem) (word state, int len) {
	char * buf = (char *)umalloc (len);
	if (buf == NULL) {
		app_diag (D_WARNING, "No mem %d", len);
		if (state != WNONE) {
			umwait (state);
			release;
		}
	}
	return buf;
}

__PUBLF (NodePeg, void, init_tag) (word i) {
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

__PUBLF (NodePeg, void, init_ign) (word i) {
	ignArray[i].id = 0;
	ignArray[i].nick[0] = '\0';
}

__PUBLF (NodePeg, void, init_mon) (word i) {
	monArray[i].id = 0;
	monArray[i].nick[0] = '\0';
}

__PUBLF (NodePeg, void, init_tags) () {
	word i = LI_MAX;
	while (i-- > 0) {
		init_tag (i);
		init_ign (i);
		init_mon (i);
	}
}

__PUBLF (NodePeg, void, set_tagState) (word i, tagStateType state,
							Boolean updEvTime) {
	tagArray[i].state = state;
	tagArray[i].lastTime = seconds();
	if (updEvTime)
		tagArray[i].evTime = tagArray[i].lastTime;
	app_diag (D_DEBUG, "set_tagState in %u to %u", i, state);
}

__PUBLF (NodePeg, int, insert_tag) (char * buf) {
	int i = 0;

	while (i < LI_MAX) {
		if (tagArray[i].id == 0) {
			tagArray[i].id = in_header(buf, snd);
			tagArray[i].profi = in_profi(buf, profi);
			set_tagState (i, newTag, YES);
			strncpy (tagArray[i].nick, in_profi(buf, nick), NI_LEN);
			tagArray[i].pl = in_profi(buf, pl);
			tagArray[i].intim = in_header(buf, rcv) != 0 ? 1 : 0;
			app_diag (D_DEBUG, "Inserted tag %u at %u",
					in_header(buf, snd), i);
			return i;
		}
		i++;
	}
	app_diag (D_SERIOUS, "Failed tag (%u) insert", in_header(buf, snd));
	return -1;
}

__PUBLF (NodePeg, int, insert_ign) (word id, char * s) {
	int i = 0;

	while (i < LI_MAX) {
		if (ignArray[i].id == 0) {
			ignArray[i].id = id;
			strncpy (ignArray[i].nick, s, NI_LEN);
			app_diag (D_DEBUG, "Inserted ign %u at %u",
					id, i);
			return i;
		}
		i++;
	}
	app_diag (D_SERIOUS, "Failed ign (5u) insert", id);
	return -1;
}

__PUBLF (NodePeg, int, insert_mon) (word id, char * s) {
	int i = 0; 

	while (i < LI_MAX) {
		if (monArray[i].id == 0) {
			monArray[i].id = id;
			strncpy (monArray[i].nick, s, NI_LEN);
			app_diag (D_DEBUG, "Inserted mon %u at %u",
					id, i);
			return i;
		}
		i++;
	}
	app_diag (D_SERIOUS, "Failed mon (%u) insert", id);
	return -1;
}

__PUBLF (NodePeg, void, check_tag) (word i) {

	if (i >= LI_MAX) {
		app_diag (D_FATAL, "tagAr bound %u", i);
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
			app_diag (D_DEBUG, "Delete %u", tagArray[i].id);
			init_tag (i);
			break;

		case goneTag:
			oss_profi_out (i);
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
			set_tagState (i, goneTag, YES);
			oss_profi_out (i);
			init_tag (i);
			break;

		default:
			app_diag (D_SERIOUS, "noTag? %u in %u",
				tagArray[i].id, tagArray[i].state);
	}
}

__PUBLF (NodePeg, void, send_msg) (char * buf, int size) {
	// it doesn't seem like a good place to filter out
	// local host, but it's convenient, for now...

	// this shouldn't be... WARNING to see why it is needed...
	if (in_header(buf, rcv) == local_host) {
		app_diag (D_WARNING, "Dropped msg(%u) to lh",
			in_header(buf, msg_type));
		return;
	}

	if (net_tx (WNONE, buf, size, 0) == 0) {
		app_diag (D_DEBUG, "Sent %u to %u",
			in_header(buf, msg_type),
			in_header(buf, rcv));
	} else
		app_diag (D_SERIOUS, "Tx %u failed",
			in_header(buf, msg_type));
 }

__PUBLF (NodePeg, int, check_msg_size) (char * buf, word size, word repLevel) {
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

