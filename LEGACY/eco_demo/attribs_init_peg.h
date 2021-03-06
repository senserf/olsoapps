/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/

// see comments in global_pegs.h
	_da (ui_ibuf)		= NULL;
	_da (ui_obuf)		= NULL;
	_da (cmd_line)		= NULL;

	_da (host_id)		= (lword) preinit ("HID");
	_da (app_flags)		= DEF_APP_FLAGS;

	_da (master_ts)		= 0;
	_da (host_pl)		= 7;

	_da (tag_auditFreq)	= 59;	// in seconds

	_da (msg4tag).buf	= NULL;
	_da (msg4tag).tstamp	= 0;

	_da (msg4ward).buf	= NULL;
	_da (msg4ward).tstamp	= 0;

	_da (pong_ack).header.msg_type = msg_pongAck;

	_da (master_date)	= 0;

	_da (sync_freq)		= 0;

	_da (sat_mod)		= 0;

	_da (plot_id)		= 0;

	_da (pow_ts)		= 0;

	_da (pow_sup)		= 0;
