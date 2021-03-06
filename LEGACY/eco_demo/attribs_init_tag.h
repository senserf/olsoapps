/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/

// see globals_tag.h for comments on changes

	_da (ui_ibuf)		= NULL;
	_da (ui_obuf)		= NULL;
	_da (cmd_line)		= NULL;

	_da (host_id)		= (lword) preinit ("HID");
	_da (app_flags)		= DEF_APP_FLAGS;

	_da (pong_params).freq_maj	= 60;
	_da (pong_params).freq_min	= 5;
	_da (pong_params).pow_levels	= 0x7777;
	_da (pong_params).rx_span	= 2048; // in msec; 1 <-> always ON
	_da (pong_params).rx_lev	= 0;
	_da (pong_params).pload_lev	= 0;

	_da (ref_ts)		= 0;
	_da (ref_date)		= 0;

	_da (plot_id)		= 0;
