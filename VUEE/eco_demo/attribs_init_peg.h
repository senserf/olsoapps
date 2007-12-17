// see comments in global_pegs.h
	_da (ui_ibuf)		= NULL;
	_da (ui_obuf)		= NULL;
	_da (cmd_line)		= NULL;

	_da (host_id)		= (lword) preinit ("HID");
	_da (app_flags)		= 0;

	_da (master_delta)	= 0;
	_da (host_pl)		= 7;

	_da (tag_auditFreq)	= 10240;	// in bin msec
	_da (tag_eventGran)	= 10;		// in seconds

	_da (msg4tag).buf	= NULL;
	_da (msg4tag).tstamp	= 0;

	_da (msg4ward).buf	= NULL;
	_da (msg4ward).tstamp	= 0;

	_da (pong_ack).header.msg_type = msg_pongAck;

