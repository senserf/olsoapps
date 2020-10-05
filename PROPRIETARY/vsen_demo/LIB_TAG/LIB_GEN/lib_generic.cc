/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/

#include "app_tag_data.h"
#include "diag.h"
#include "oss_fmt.h"
#include "form.h"

fsm sread {

	state S0:
#ifdef SENSOR_LIST
		read_sensor (S0, -2, &sens_data.ee.sval[0]);
	state S1:
		read_sensor (S1, -1, &sens_data.ee.sval[1]);
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
        "%s slot %lu %u-%u-%u %u:%u:%u "
        "%d %d %d\r\n";

char * form_dump (sensEEDumpType *sd, const char *nam, mdate_t *md) {

        return form (NULL, dump_str, nam, sd->ind,

                        md->dat.f ? 2009 + md->dat.yy : 1001 + md->dat.yy,
                        md->dat.mm, md->dat.dd, md->dat.h, md->dat.m, md->dat.s,

                        sd->ee.sval[0], sd->ee.sval[1],
			sd->ee.s.f.setsen);
}

