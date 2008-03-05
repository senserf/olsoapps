// see comments in global_pegs.h
	_da (ui_ibuf)		= NULL;
	_da (ui_obuf)		= NULL;
	_da (cmd_line)		= NULL;

	_da (host_id)		= (lword) preinit ("HID");
	_da (app_flags)		= 0;

	_da (host_pl)		= 1;

	_da (tag_auditFreq)	= 4;
	_da (tag_eventGran)	= 4;

	_da (profi_att)		= (word) preinit ("PROFI");
	_da (p_inc)		= (word) preinit ("PINC");
	_da (p_exc)		= (word) preinit ("PEXC");

	_da (led_state).color	= LED_N;
	_da (led_state).state	= LED_OFF;
	_da (led_state).dura	= 0;
#if 0
	this is ok, but not needed (inited explicite)
	for (int i = 0; i < LI_MAX; i++) {
		_da (l_ign) [i] = 0;
		_da (l_mon) [i] = 0;
	}
#endif
	strcpy (_da (nick_att), (char *) preinit ("NICK"));
	strcpy (_da (desc_att), (char *) preinit ("DESC"));
	strcpy (_da (d_biz), (char *) preinit ("DBIZ"));
	strcpy (_da (d_priv), (char *) preinit ("DPRIV"));
	_da (d_alrm[0]) = '\0';

