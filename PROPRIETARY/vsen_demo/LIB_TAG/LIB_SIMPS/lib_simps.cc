/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2012                           */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "app_tag_data.h"
#include "diag.h"
#include "oss_fmt.h"
#include "form.h"
#include "sensets.h"

// quite obvious: it should be done in a fundamentally different way...
// it will be, one day...
#ifdef SENSOR_LIST
static sint cursen (word which) {
	switch (senset) {
		case SENSET_ECO_12:
			if (which == 2)
				return 1;
			return 2;
		case SENSET_ECO_45:
			if (which == 2)
				return 4;
			return 5;
	}
	//default:
	if (which == 2)
		return 0;
	return 1;
}
#endif

fsm sread {

	state S0:
#ifdef SENSOR_LIST
		read_sensor (S0, -2, &sens_data.ee.sval[0]);
	state S1:
		read_sensor (S1, -1, &sens_data.ee.sval[1]);
	state S2:
		read_sensor (S2, cursen (2), &sens_data.ee.sval[2]);
	state S3:
		read_sensor (S2, cursen (3), &sens_data.ee.sval[3]);
#else
	sint i;
	app_diag (D_WARNING, "FAKE SENSORS");
	for (i = 0; i < NUM_SENS; i++)
		sens_data.ee.sval[i] += i;

	delay (SENS_COLL_TIME, S_FIN);
	release;
#endif

	state S_FIN:
		sens_data.ee.s.f.status = SENS_COLLECTED;
		sens_data.ee.s.f.mark = MARK_FF;
		trigger (SENS_DONE);
		finish;
}

static trueconst char dump_str[] = OPRE_APP_DUMP_C OMID_CR
        "%s slot %lu %u-%u-%u %u:%u:%u " "%d %d %d %d %d\r\n";

char * form_dump (sensEEDumpType *sd, const char * nam, const mdate_t * md) {

        return form (NULL, dump_str, nam, sd->ind,

                        md->dat.f ? 2009 + md->dat.yy : 1001 + md->dat.yy,
                        md->dat.mm, md->dat.dd, md->dat.h, md->dat.m, md->dat.s,

                        sd->ee.sval[0], sd->ee.sval[1],
			sd->ee.sval[2], sd->ee.sval[3],
			sd->ee.s.f.setsen);
}

