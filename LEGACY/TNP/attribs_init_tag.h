/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/

	_da (ui_ibuf)		= NULL;
	_da (ui_obuf)		= NULL;
	_da (cmd_line)		= NULL;

	_da (host_id)		= (lword) preinit ("HID");
	_da (host_passwd)	= 0;
	_da (app_flags)		= 0;

	_da (app_count).rcv	= 0;
	_da (app_count).snd	= 0;
	_da (app_count).fwd	= 0;

	_da (pong_params).freq_maj	= 5120;
	_da (pong_params).freq_min	= 1024;
	_da (pong_params).pow_levels	= 0x0543;
	_da (pong_params).rx_span	= 1024;
	_da (pong_params).rx_lev	= 5;
