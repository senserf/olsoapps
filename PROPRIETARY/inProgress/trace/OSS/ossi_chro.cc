/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2012                           */
/* All rights reserved.                                                 */
/* ==================================================================== */

/* ser -specific ossi (setting praxis-specific cmd_d struct) */

#include "ser.h"
#include "serf.h"

fsm ossi_init {
	byte b;

	state OI_WATCHO:
		ser_select (1):
			etc.
		ser_out (OI_WATCHO, "Anybody? (Y)\r\n");
		delay (60*1024, OI_WATCHO);

	state OI_WATCHI:
                ser_in (OI_WATCHI, b, 1);
	                if (b == 'Y') {
				runfsm ossi_in;
				finish;
			}
			proceed (OI_WATCHI);
}

fsm ossi_out (char *) {
	cmd_t * out_d = (cmd_t *)data;

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
		// note that ser_* takes care of .buf
		// operations (sure SEGV or mem leaks if wrong called)
		ufree (out_d);
		finish;
}

static int ibuf2cmd (cod_t * in_cmd, char * buf) {
	cod_t * cmd;
	char  * s1, * s2;
	sint i;

	if ((cmd = get_mem (WNONE, sizeof(cmd_t))) == NULL)
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
			s1 = buf + strlen (buf) - i;
		}

		if ((s2 = get_mem (WNONE, i)) == NULL)
			goto Crap;

		strcpy (s2, s1);
		cmd->buf = s2;
	}

	if (cmd->code == 'a') { // rpc / any: recursion and serialization
		if (ibuf2cmd (cmd, buf + strlen (buf) - cmd->argv_w[0] -1)) {
			goto Crap;
		}
	}

	if (in_cmd) { // this means serialization (any / rpc) to send out

		if (cmd->buf) {
			i = strlen (cmd->buf) +1;
			s2 = cmd->buf;
			cmd->buf = i;
		else {
			i = 0;
			s2 = NULL;
		}

		i += 2* sizeof(cmd_t);
		if ((s1 = get_mem (WNONE, i)) == NULL)
			goto Crap;

		in_cmd->size = sizeof(cmd_t) + cmd->size;
		memcpy (s1, in_cmd, sizeof (cmd_t));
		mamcpy (s1 + sizeof (cmd_t), cmd, sizeof (cmd_t));
		if (s2)
			strcpy (s1 + 2* sizeof (cmd_t), s2);

		ufree (s2);
		ufree (cmd);
		ufree (in_cmd);
		in_cmd = s1;
	} else
		in_cmd = cmd;

	return 0;

Crap:
	ufree (cmd);
	return 1;
}


fsm ossi_in {
	char * ibuf = NULL;

	state OI_INIT:
		ibuf = get_mem (OI_INIT, UART_INPUT_BUFFER_LENGTH);

	state OI_CLR:
		if (app_flags.oss_a == 0) {
			ufree (ibuf);
			runfsm oss_init;
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
		req_t * req = get_mem (OI_CMD, sizeof (req_t));

		if (ibuf2cmd (req->cmd, ibuf)) {
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

#define m	((msgStatsType *)b)
void ossi_stats_out (b * m) {
	cmd_t * cmd = get_mem (WNONE, sizeof (cmd_t));
	word w[2];

	if (cmd == NULL)
		return;

	cmd->code = 'b';

	if (m) {

	} else {
		w[0] =  memfree (0, &w[1]);

		cmd->buf = form (NULL, stats_str, local_host, app_flags,
			       	seconds(), master_host, w[0], w[1],
			       	stackfree(), bat);
	}

	if (runfsm oss_out ((char *)cmd) == 0) {
		diag ("fork oss");
		ufree (cmd->buf);
		ufree (cmd);
	}
}
#undef m

void ossi_trace_out (char * buf, word rssi) {
        sint i, num = 0, cnt = 0;
        char * ptr = buf;
	char * lines[];
	cmd_t * cmd;

	if ((cmd = get_mem (WNONE, sizeof (cmd_t))) == NULL)
		return;

	ptr += (in_header(buf, msg_type) == msg_trace1) ?
		sizeof (msgTraceType) : sizeof (msgTraceAckType);

        if (in_header(buf, msg_type) != msg_traceBAck &&
			in_header(buf, msg_type) != msg_trace1)
                num = in_traceAck(buf, fcount);
        if (in_header(buf, msg_type) != msg_traceFAck)
                num += in_header(buf, hoc) & 0x7F;
        if (in_header(buf, msg_type) == msg_traceAck)
                num--; // dst counted twice

	// num + 1st, last in words
	if ((lines =  get_mem (WNONE, 2*(num +2))) == NULL)
		goto Cleanup;

	if ((lines[cnt++] = form (NULL, "tr(%u) %u %u %u:\r\n",
			in_header(buf, snd),
                        in_header(buf, msg_type),
                        in_traceAck(buf, fcount),
                        in_header(buf, hoc) & 0x7F)) == NULL)
		goto Cleanup;

        while (num--) {
		if ((lines[cnt++] = form (NULL, " %u %u\r\n",
				*ptr, *(ptr +1))) == NULL)
			goto Cleanup;
                // careful with (... *ptr++, *ptr++) instead
                ptr += 2;
	}
	if ((lines[cnt++] = form (NULL, " %u %u\r\n", local_host, rssi)) ==
		       	NULL)
		goto Cleanup;

	// so, we have the lines[] loaded up to cnt-1. cmd->buf will hold
	// formatted output:
	num = 0;
	for (i = 0; i < cnt; i++)
		num += strlen (lines[i]);

	if ((cmd->buf = get_mem (WNONE, num +1)) == NULL) // +1 for '\0'
		goto Cleanup;

	for (i = 0; i < cnt; i++) {
		strcat (cmd->buf, lins[i]);
		ufree (lines[i]);
	}
	ufree (lines);

	if (runfsm ossi_out ((char *)cmd) == 0) {
		diag ("fork oss");
		ufree (cmd->buf);
		ufree (cmd);
	}
	return;

Cleanup:
	while (cnt--)
		ufree (lines[cnt]);
	ufree (lines);
	ufree (cmd);
}

