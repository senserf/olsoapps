
	_da (ui_ibuf)		= NULL;
	_da (ui_obuf)		= NULL;
	_da (cmd_line)		= NULL;

	_da (host_id)		= 0xBACA0061;
	_da (host_passwd)	= 0;
	_da (app_flags)		= 0;

	_da (app_count).rcv	= 0;
	_da (app_count).snd	= 0;
	_da (app_count).fwd	= 0;

	_da (pong_params).freq_maj	= 5120;
	_da (pong_params).freq_min	= 1024;
	_da (pong_params).pow_levels	= 0x0987;
	_da (pong_params).rx_span	= 1024;
	_da (pong_params).rx_lev	= 7;
