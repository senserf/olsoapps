/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2012                           */
/* All rights reserved.                                                 */
/* ==================================================================== */

/* ser -specific ossi (setting praxis-specific cmd_d struct) */

#include "ser.h"
#include "serf.h"
#include "app.h"
#include "app_dcl.h"
#include "oss.h"
#include "oss_dcl.h"
#include "str_ser.h"
#include "msg.h"
#include "msg_dcl.h"

// dupa: have we stated / explained that static fsm's are illegal?
fsm ossi_in;

fsm ossi_init {
	char b[2]; // this is minimum for a non-empty NULL-terminated string

	state OI_WATCHO:
		ser_outf (OI_WATCHO, init_str, seconds());

	state OI_WATCHD:
		delay (OSS_QUANT, OI_WATCHO);

	state OI_WATCHI:
                ser_in (OI_WATCHI, b, 2); // this will copy just 1 char

	                if (b[0] == 'Y') {
				app_flags.f.oss_a = YES;
				runfsm ossi_in;
				finish;
			}
			proceed (OI_WATCHO);
}

static void run_oo (cmd_t * c) {
	if (runfsm ossi_out ((char *)c) == 0) {
		diag ("fork oss");
		if (c->code == 'b')
			ufree (c->buf);
		ufree (c);
	}
}

#define out_d	((cmd_t *)data)
fsm ossi_out (char *) {

	entry OO_INIT:
		if (data == NULL || out_d->buf == NULL)
			goto Fin;

		// if the actual buffer contains empty string, it is interpreted
		// as a binary command... and bad things happen
		if (*(out_d->buf) == '\0') {
			if (out_d->code == 'b') {
				ufree (out_d->buf);
			}
			goto Fin;
		}		

	entry OO_START:
		switch (out_d->code) {
			case ' ':
				ser_out (OO_START, out_d->buf);
				break;

			case 'b':
				ser_outb (OO_START, out_d->buf);
				break;

			case 'f':
			case 'F':
				ser_outf (OO_START, out_d->buf,
					out_d->argv_w[0],
					(int)out_d->argv_w[1],
					out_d->argv_w[2]);
				if (out_d->code == 'F')
					ufree (out_d->buf);
				break;
		}
Fin:
		// note that ser_* takes care of .buf
		// operations (sure SEGV or mem leaks if wrong called)
		ufree (out_d);
		finish;
}
#undef out_d

static int ibuf2cmd (cmd_t ** in_cmd, char * buf) {
	cmd_t * cmd;
	char  * s1, * s2;
	sint i;

	if ((cmd = (cmd_t *)get_mem (WNONE, sizeof(cmd_t))) == NULL)
		goto Crap;

	cmd->code = buf[0];

	// special case: hex for 
	if (cmd->code == 'T') // hex for tarp flags
		cmd->arg_c = scan (buf +1, "%x", &cmd->argv_w[0]);
	else // scan in args (excessive, but in one place)
		cmd->arg_c = scan (buf +1, "%u %u %u", &cmd->argv_w[0],
				&cmd->argv_w[1], &cmd->argv_w[2]);

	// 'string' arg
	if (cmd->code == ' ' || cmd->code == 'd') {
		if (cmd->code == ' ') {
			i = strlen (buf);
			s1 = buf +1;
		} else {
			i = cmd->argv_w[0] +1;
			s1 = buf + strlen (buf) - i +1;
		}

		if ((s2 = get_mem (WNONE, i)) == NULL)
			goto Crap;

		strcpy (s2, s1);
		cmd->buf = s2;
	}

	if (cmd->code == 'a') { // rpc / any: recursion and serialization

		if (*in_cmd != NULL) {
			diag ("nested a");
			goto Crap;
		}

		if (cmd->arg_c < 3 || cmd->argv_w[2] == 0 ||
				strlen (buf) < cmd->argv_w[2] +7) {
			// 7:(a<dst> <hops> <len> )here comes <rpc>
			diag ("a %d %d", cmd->arg_c, 
				strlen (buf) - cmd->argv_w[2]);
			goto Crap;
		}

		if (ibuf2cmd (&cmd, buf + strlen (buf) - cmd->argv_w[2]))
				goto Crap;
	}


	if (*in_cmd) { // this means serialization (any / rpc) to send out
		if (cmd->buf) {
			i = strlen (cmd->buf) +1;
			s2 = cmd->buf;
			cmd->size = i;
		} else {
			i = 0;
			s2 = NULL;
		}
		i += 2* sizeof(cmd_t);
		if ((s1 = get_mem (WNONE, i)) == NULL)
			goto Crap;

		(*in_cmd)->size = sizeof(cmd_t) + cmd->size;
		memcpy (s1, (char *)*in_cmd, sizeof (cmd_t));
		memcpy (s1 + sizeof (cmd_t), (char *)cmd, sizeof (cmd_t));
		if (s2)
			strcpy (s1 + 2* sizeof (cmd_t), s2);

		ufree (s2);
		ufree (cmd);
		ufree (*in_cmd);
		*in_cmd = (cmd_t *)s1;
	} else {
		*in_cmd = cmd;
	}

	return 0;

Crap:
	ufree (cmd);
	return 1;
}


fsm ossi_in {
	char * ibuf = NULL;

	state OI_INIT:
		ser_out (OI_INIT, welcome_str);
		ibuf = get_mem (OI_INIT, UART_INPUT_BUFFER_LENGTH);

	state OI_CLR:
		if (app_flags.f.oss_a == NO) {
			ufree (ibuf);
			runfsm ossi_init;
			finish;
		}
		ibuf[0] = '\0'; // clear ibuf

	state OI_IN:
		when (TRIG_OSSI, OI_CLR);
		ser_in (OI_IN, ibuf, UART_INPUT_BUFFER_LENGTH);
		if (strlen(ibuf) == 0)
			// CR on empty line would do it
			proceed OI_IN;

	state OI_CMD:
		req_t * req = (req_t *)get_mem (OI_CMD, sizeof (req_t));

		if (ibuf2cmd (&req->cmd, ibuf)) {
			ufree (req);
			proceed OI_ILL;
		}

		if (req_in (req))
			proceed OI_ILL;

		proceed OI_CLR;

	 state OI_ILL:
		ser_outf (OI_ILL, ill_str, ibuf);
	 	proceed OI_CLR;

}

void ossi_help_out () {
	cmd_t * cmd = (cmd_t *)get_mem (WNONE, sizeof (cmd_t));
	if (cmd == NULL)
		return;

	cmd->code = ' ';
	cmd->buf = (char *)welcome_str;
	run_oo (cmd);
}

void ossi_beac_out () {
	cmd_t * cmd = (cmd_t *)get_mem (WNONE, sizeof (cmd_t));
	if (cmd == NULL)
		return;

	cmd->code = 'b';
	cmd->buf = form (NULL, beac_str, beac.f, beac.s, 
			beac.b ? in_header(beac.b,msg_type) : 0);

	if (cmd->buf == NULL) {
		ufree (cmd);
		return;
	}
	run_oo (cmd);
}

#define m	((msgStatsType *)b)
void ossi_stats_out (char * b) {
	cmd_t * cmd = (cmd_t *)get_mem (WNONE, sizeof (cmd_t));
	word w[2];

	if (cmd == NULL)
		return;

	cmd->code = 'b';

	if (m) {

		cmd->buf = form (NULL, stats_str, in_header(m, snd),
				in_stats(m, fl), in_stats(m, ltime),
				in_stats(m, mhost), in_stats(m, mem),
				in_stats(m, mmin), in_stats(m, stack),
				in_stats(m, batter));

	} else {
		w[0] =  memfree (0, &w[1]);

		cmd->buf = form (NULL, stats_str, local_host, app_flags.w,
			       	seconds(), master_host, w[0], w[1],
			       	stackfree(), bat);
	}

	run_oo (cmd);
}
#undef m

void ossi_trace_out (char * buf, word rssi) {
        sint i, num = 0, cnt = 0;
        char * ptr = buf;
	char **lines;
	cmd_t * cmd;

	if ((cmd = (cmd_t*)get_mem (WNONE, sizeof (cmd_t))) == NULL)
		return;

	ptr += (in_header(buf, msg_type) == msg_trace1) ?
		sizeof (msgTraceType) : sizeof (msgTraceAckType);

        if (in_header(buf, msg_type) != msg_traceBAck &&
			in_header(buf, msg_type) != msg_trace1)
                num = in_traceAck(buf, fcount);
        if (in_header(buf, msg_type) != msg_traceFAck)
                num += in_header(buf, hoc) & 0x7F;
        if (in_header(buf, msg_type) == msg_traceAck ||
		in_header(buf, msg_type) == msg_trace1)
                num--; // dst counted twice

	// num + 1st, last in words
	if ((lines =  (char **)get_mem (WNONE, sizeof(char *)*(num +2))) ==
			 NULL)
		goto Cleanup;

	if ((lines[cnt++] = form (NULL, "tr(%u) %u %u %u:\r\n",
			in_header(buf, snd),
                        in_header(buf, msg_type),
                        (in_header(buf, msg_type) == msg_trace1) ?
				in_header(buf, seq_no) : // this is handy
				in_traceAck(buf, fcount),
                        in_header(buf, hoc) & 0x7F)) == NULL)
		goto Cleanup;

        while (num--) {
		if ((lines[cnt++] = form (NULL, " %u %u%c\r\n",
				*(byte *)ptr, *(byte *)(ptr +1),
			cnt == 1 && in_header(buf, msg_type) == msg_traceBAck ?
				'*' : ' ')) == NULL)
			goto Cleanup;
                // careful with (... *ptr++, *ptr++) instead
                ptr += 2;
	}

	if ((lines[cnt++] = form (NULL, "%c%u %u\r\n",
			in_header(buf, msg_type) == msg_traceFAck ? '*' : ' ',
			local_host, rssi)) == NULL)
		goto Cleanup;

	// so, we have the lines[] loaded up to cnt-1. cmd->buf will hold
	// formatted output:
	num = 0;
	for (i = 0; i < cnt; i++)
		num += strlen (lines[i]);

	if ((cmd->buf = get_mem (WNONE, num +1)) == NULL) // +1 for '\0'
		goto Cleanup;

	for (i = 0; i < cnt; i++) {
		strcat (cmd->buf, lines[i]);
		ufree (lines[i]);
	}
	ufree (lines);

	cmd->code = 'b';
	run_oo (cmd);
	return;

Cleanup:
	while (cnt--)
		ufree (lines[cnt]);
	ufree (lines);
	ufree (cmd);
}

