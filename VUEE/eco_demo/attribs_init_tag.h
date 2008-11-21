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

	_da (ref_time)		= 0;
	_da (ref_clock).sec	= 0;
