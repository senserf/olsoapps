// see comments in global_pegs.h
	_da (ui_ibuf)		= NULL;
	_da (ui_obuf)		= NULL;
	_da (cmd_line)		= NULL;

	_da (host_id)		= (lword) preinit ("HID");
	_da (app_flags)		= DEF_APP_FLAGS;

	_da (sstate)		= ST_OFF;

	_da (oss_stack)		= 0;

	_da (touts).ts		= 0;
	_da (touts).mc		= 0;
	_da (touts).iv.b.sil	= DEF_I_SIL;
	_da (touts).iv.b.warm	= DEF_I_WARM;
	_da (touts).iv.b.cyc	= DEF_I_CYC;

