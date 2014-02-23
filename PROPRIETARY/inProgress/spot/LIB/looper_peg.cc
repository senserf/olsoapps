/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2014                           */
/* All rights reserved.                                                 */
/* ==================================================================== */

/* Reusable module looping for heartbeats (tagList audit in this incarnation).
   Heartbeat == 0 calls a single pass through tagList with calls to report_tag.
*/

#include "sysio.h"
#include "commons.h"
#include "diag.h"
#include "tag_mgr.h"
#include "hold.h"

word	heartbeat = 25; // seconds

#define _LOO_DBG	0

/* we want to spread re-reporting of multiple tags, but we can't hang on
   a given list element, as it can be deallocated any moment. So, we have
   a binary audit marker (marka) and quickly pick the next element from'
   the head
*/
static char * nextel () {
	char * ptr = tagList.nel;
 
	while (ptr && (in_tdt(ptr, marka) == tagList.marka ||
		(word)(seconds() - in_tdt(ptr, refTime)) < heartbeat)) {
#if _LOO_DBG
		app_diag_U ("LOO: skipped %u#%u", in_tdt(ptr, tagid),
				in_tdt(ptr, refTime));
#endif
		ptr = in_tdt(ptr, nel);
	}

	return ptr;
}

fsm looper {
        lword htime;

        state BEG:
#if _LOO_DBG
		app_diag_U ("LOO: BEG (%u) %u+%u", (word)seconds(),
			tagList.alrms, tagList.evnts);
#endif
                htime = seconds() + heartbeat;
		tagList.marka++;	// flip

        state TAG:
		char * tp = nextel();
		if (tp) {
#if _LOO_DBG
			app_diag_U ("LOO: in %u#%u", in_tdt(tp, tagid),
				in_tdt(tp, refTime));
#endif
			in_tdt(tp, marka) = tagList.marka;
			report_tag (tp);
			delay (1024, TAG);
			release;
		}

		if (heartbeat == 0) {
			app_diag_W ("looper exits");
			finish;
		}
#if _LOO_DBG
		app_diag_U ("LOO: hold %u", (word)(htime - seconds()));
#endif
        state HOLD:
                hold (HOLD, htime);
                proceed BEG;
}

