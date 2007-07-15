
	_da (ui_ibuf)		= NULL;
	_da (ui_obuf)		= NULL;
	_da (cmd_line)		= NULL;

	_da (host_id)		= (lword) preinit ("HID");
	_da (host_password)	= 0;
	_da (app_flags)		= 0;

	_da (master_delta)	= 0;
	_da (host_pl)		= 9;

	_da (tag_auditFreq)	= 10240;	// in bin msec
	_da (tag_eventGran)	= 10;		// in seconds

	_da (app_count).rcv	= 0;
	_da (app_count).snd	= 0;
	_da (app_count).fwd	= 0;

	_da (msg4tag).buf	= NULL;
	_da (msg4tag).tstamp	= 0;

	_da (msg4ward).buf	= NULL;
	_da (msg4ward).tstamp	= 0;
