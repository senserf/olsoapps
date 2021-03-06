/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#ifndef	__attnames_peg_h__
#define	__attnames_peg_h__

#ifdef __SMURPH__

// Attribute name conversion

#define	ui_ibuf		_dac (NodePeg, ui_ibuf)
#define	ui_obuf		_dac (NodePeg, ui_obuf)
#define	cmd_line	_dac (NodePeg, cmd_line)

#define	host_id		_dac (NodePeg, host_id)
#define	host_pl		_dac (NodePeg, host_pl)
#define	master_ts	_dac (NodePeg, master_ts)
#define master_date	_dac (NodePeg, master_date)
#define	msg4tag		_dac (NodePeg, msg4tag)
#define	msg4ward	_dac (NodePeg, msg4ward)
#define	tagArray	_dac (NodePeg, tagArray)
#define	tag_auditFreq	_dac (NodePeg, tag_auditFreq)
#define	app_flags	_dac (NodePeg, app_flags)
#define agg_data	_dac (NodePeg, agg_data)
#define agg_dump	_dac (NodePeg, agg_dump)
#define pong_ack	_dac (NodePeg, pong_ack)
#define sync_freq	_dac (NodePeg, sync_freq)
#define sat_mod		_dac (NodePeg, sat_mod)
#define plot_id		_dac (NodePeg, plot_id)
#define pow_ts		_dac (NodePeg, pow_ts)
#define pow_sup		_dac (NodePeg, pow_sup)

// Methods
#define show_ifla		_dac (NodePeg, show_ifla)
#define read_ifla		_dac (NodePeg, read_ifla)
#define save_ifla		_dac (NodePeg, save_ifla)
#define	stats			_dac (NodePeg, stats)
#define	app_diag		_dac (NodePeg, app_diag)
#define	net_diag		_dac (NodePeg, net_diag)

#define	process_incoming	_dac (NodePeg, process_incoming)
#define	check_msg4tag		_dac (NodePeg, check_msg4tag)
#define	check_msg_size		_dac (NodePeg, check_msg_size)
#define	check_tag		_dac (NodePeg, check_tag)
#define	find_tags		_dac (NodePeg, find_tags)
#define	get_mem			_dac (NodePeg, get_mem)
#define	init_tag		_dac (NodePeg, init_tag)
#define	init_tags		_dac (NodePeg, init_tags)
#define	insert_tag		_dac (NodePeg, insert_tag)
#define	set_tagState		_dac (NodePeg, set_tagState)

#define	msg_findTag_in		_dac (NodePeg, msg_findTag_in)
#define	msg_findTag_out		_dac (NodePeg, msg_findTag_out)
#define	msg_master_in		_dac (NodePeg, msg_master_in)
#define	msg_master_out		_dac (NodePeg, msg_master_out)
#define	msg_pong_in		_dac (NodePeg, msg_pong_in)
#define	msg_reportAck_in	_dac (NodePeg, msg_reportAck_in)
#define	msg_reportAck_out	_dac (NodePeg, msg_reportAck_out)
#define	msg_report_in		_dac (NodePeg, msg_report_in)
#define	msg_report_out		_dac (NodePeg, msg_report_out)
#define	msg_fwd_in		_dac (NodePeg, msg_fwd_in)
#define	msg_fwd_out		_dac (NodePeg, msg_fwd_out)
#define	msg_fwd_msg		_dac (NodePeg, msg_fwd_msg)
#define	copy_fwd_msg		_dac (NodePeg, copy_fwd_msg)
#define msg_setPeg_in		_dac (NodePeg, msg_setPeg_in)

#define	oss_findTag_in		_dac (NodePeg, oss_findTag_in)
#define	oss_setTag_in		_dac (NodePeg, oss_setTag_in)
#define	oss_setPeg_in		_dac (NodePeg, oss_setPeg_in)
#define	oss_master_in		_dac (NodePeg, oss_master_in)
#define	oss_report_out		_dac (NodePeg, oss_report_out)

#define	send_msg		_dac (NodePeg, send_msg)

#define agg_init		_dac (NodePeg, agg_init)
#define fatal_err		_dac (NodePeg, fatal_err)
#define write_agg		_dac (NodePeg, write_agg)
#define write_mark		_dac (NodePeg, write_mark)
#define r_a_d			_dac (NodePeg, r_a_d)
#define handle_a_flags		_dac (NodePeg, handle_a_flags)
#define tmpcrap			_dac (NodePeg, tmpcrap)
#define str_cmpn		_dac (NodePeg, str_cmpn)
#define sat_rep			_dac (NodePeg, sat_rep)
#define sat_out			_dac (NodePeg, sat_out)
#define wall_date		_dac (NodePeg, wall_date)

#else	/* PICOS */

#include "attribs_peg.h"
#include "tarp.h"

#endif	/* SMURPH or PICOS */

#endif
