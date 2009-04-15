#ifndef	__attnames_cus_h__
#define	__attnames_cus_h__

#ifdef __SMURPH__

// Attribute name conversion

#define	ui_ibuf		_dac (NodeCus, ui_ibuf)
#define	ui_obuf		_dac (NodeCus, ui_obuf)
#define	cmd_line	_dac (NodeCus, cmd_line)

#define	host_id		_dac (NodeCus, host_id)
#define	host_pl		_dac (NodeCus, host_pl)
#define	master_ts	_dac (NodeCus, master_ts)
#define master_date	_dac (NodeCus, master_date)
#define	msg4tag		_dac (NodeCus, msg4tag)
#define	msg4ward	_dac (NodeCus, msg4ward)
#define	tagArray	_dac (NodeCus, tagArray)
#define	tag_auditFreq	_dac (NodeCus, tag_auditFreq)
#define	app_flags	_dac (NodeCus, app_flags)
#define agg_data	_dac (NodeCus, agg_data)
#define agg_dump	_dac (NodeCus, agg_dump)
#define pong_ack	_dac (NodeCus, pong_ack)
#define sync_freq	_dac (NodeCus, sync_freq)
#define sat_mod		_dac (NodeCus, sat_mod)
#define plot_id		_dac (NodeCus, plot_id)

// Methods
#define show_ifla		_dac (NodeCus, show_ifla)
#define read_ifla		_dac (NodeCus, read_ifla)
#define save_ifla		_dac (NodeCus, save_ifla)
#define	stats			_dac (NodeCus, stats)
#define	app_diag		_dac (NodeCus, app_diag)
#define	net_diag		_dac (NodeCus, net_diag)

#define	process_incoming	_dac (NodeCus, process_incoming)
#define	check_msg4tag		_dac (NodeCus, check_msg4tag)
#define	check_msg_size		_dac (NodeCus, check_msg_size)
#define	check_tag		_dac (NodeCus, check_tag)
#define	find_tags		_dac (NodeCus, find_tags)
#define	get_mem			_dac (NodeCus, get_mem)
#define	init_tag		_dac (NodeCus, init_tag)
#define	init_tags		_dac (NodeCus, init_tags)
#define	insert_tag		_dac (NodeCus, insert_tag)
#define	set_tagState		_dac (NodeCus, set_tagState)

#define	msg_findTag_in		_dac (NodeCus, msg_findTag_in)
#define	msg_findTag_out		_dac (NodeCus, msg_findTag_out)
#define	msg_master_in		_dac (NodeCus, msg_master_in)
#define msg_satest_out		_dac (NodeCus, msg_satest_out)
#define msg_rpc_out		_dac (NodeCus, msg_rpc_out)
#define sat_in			_dac (NodeCus, sat_in)
#define sat_out			_dac (NodeCus, sat_out)
#define	msg_pong_in		_dac (NodeCus, msg_pong_in)
#define	msg_reportAck_in	_dac (NodeCus, msg_reportAck_in)
#define	msg_reportAck_out	_dac (NodeCus, msg_reportAck_out)
#define	msg_report_in		_dac (NodeCus, msg_report_in)
#define	msg_report_out		_dac (NodeCus, msg_report_out)
#define	msg_fwd_in		_dac (NodeCus, msg_fwd_in)
#define	msg_fwd_out		_dac (NodeCus, msg_fwd_out)
#define	msg_fwd_msg		_dac (NodeCus, msg_fwd_msg)
#define	copy_fwd_msg		_dac (NodeCus, copy_fwd_msg)
#define msg_setPeg_in		_dac (NodeCus, msg_setPeg_in)

#define	oss_findTag_in		_dac (NodeCus, oss_findTag_in)
#define	oss_setTag_in		_dac (NodeCus, oss_setTag_in)
#define	oss_setPeg_in		_dac (NodeCus, oss_setPeg_in)
#define	oss_report_out		_dac (NodeCus, oss_report_out)

#define	send_msg		_dac (NodeCus, send_msg)

#define agg_init		_dac (NodeCus, agg_init)
#define fatal_err		_dac (NodeCus, fatal_err)
#define write_agg		_dac (NodeCus, write_agg)
#define r_a_d			_dac (NodeCus, r_a_d)
#define handle_a_flags		_dac (NodeCus, handle_a_flags)
#define tmpcrap			_dac (NodeCus, tmpcrap)
#define str_cmpn		_dac (NodeCus, str_cmpn)
#define wall_date		_dac (NodeCus, wall_date)

#else	/* PICOS */

#include "attribs_cus.h"
#include "tarp.h"

#endif	/* SMURPH or PICOS */

#endif
