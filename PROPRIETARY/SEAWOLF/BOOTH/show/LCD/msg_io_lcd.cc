/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/

#include "app_lcd_data.h"
#include "msg.h"

#include "net.h"


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

	send_msg (buf_out, sizeof (msgAlrmType));
	ufree (buf_out);
}

