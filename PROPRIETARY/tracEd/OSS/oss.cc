/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2012                           */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "oss.h"
#include "app.h"
#include "app_dcl.h"
#include "oss_dcl.h"
#include "msg_dcl.h"
#include "net.h"

void rel_cmd (cmd_t * cmd) {
	if (cmd->code != 'a' && cmd->buf)
		ufree (cmd->buf); // 'a' was allocated in 1 piece
	ufree (cmd);
}

/*
- Caller must NOT free req->cmd nor req
- If we ever want to put req on a beacon, this rather belongs to OSS
  (not so with periodic msgs... dupa: I'm not sure if this distinction is
   convincing)
*/ 
sint req_in (req_t * req) {
	sint rc = 0, st = 0;

	if (req == NULL || req->cmd == NULL) {
		ufree (req); // doesn't hurt
		return 1;
	}

	switch (req->cmd->code) {
		case 's':
			if (req->src == 0) { // not from remote 'a'

				req->src = req->cmd->argv_w[0];
				if (req->cmd->arg_c > 0 && req->src == 0)
					// kludgy bcast
					req->src = 0xFFFF;
			}
			st = 1;
			break;
		case 'h':
			ossi_help_out();
			ufree (req->cmd);
			break;
		case 'q':
			reset();
		case 'O':
			app_flags.f.oss_a = 0;
			trigger (TRIG_OSSI);
			st = 1;
			break;
		case 'd':
		case ' ':
			req->cmd->code = 'b';
			runfsm ossi_out ((char *)req->cmd);
			break;
		case 't':
			if (msg_trace_out (req->cmd->argv_w[0],
					       	req->cmd->argv_w[1],
						req->cmd->argv_w[2]))
				rc = 1;

			rel_cmd (req->cmd);
			break;

		case 'a':
			if (req->cmd->argv_w[0] == local_host 
					|| msg_any_out (req))
				rc = 1;

			rel_cmd (req->cmd);
			break;

		case 'r':
			// dupa: with somewhat redundant rx flag, we can avoid
			// 'empty' on/off calls. Should we?
			if (req->cmd->arg_c > 0) {
				if (req->cmd->argv_w[0]) {
					net_opt (PHYSOPT_RXON, NULL);
					app_flags.f.rx = 1;
				} else {
					net_opt (PHYSOPT_RXOFF, NULL);
					app_flags.f.rx = 0;
				}
			}
			st = 1;
			break;

		case 'm':
			if (req->cmd->arg_c > 0) { 
				if (req->cmd->argv_w[0]) {
					master_host = local_host;
					leds (LED_G, LED_OFF);
					if (!running (mbeacon))
						runfsm mbeacon;
					else
						trigger (TRIG_MASTER);
				 } else {
					 master_host = 0;
					 killall (mbeacon);
					 master_ts= seconds();
					 highlight_clear();
				 }
			}
			st = 1;
			break;

		case 'b':
			if (req->cmd->arg_c > 0) {
				killall (msgbeac);
				ufree (beac.b); beac.b = NULL;
				beac.s = 0;
				if (req->cmd->argv_w[0] &&
						req->cmd->argv_w[0] < 64)
					beac.f = req->cmd->argv_w[0];
				else
					beac.f = 0;
			} else
				trigger (TRIG_BEAC);

			ossi_beac_out();
			ufree (req->cmd);
			break;
			
		default:
			rc = 1;
	}

	if (st) {

		if (req->src == 0 || req->src == local_host)
			ossi_stats_out(NULL);
		else
			rc = msg_stats_out (req->src);

		rel_cmd (req->cmd);
	}

	ufree (req);
	return rc;
}

