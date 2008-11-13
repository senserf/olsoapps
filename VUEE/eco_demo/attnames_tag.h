#ifndef	__attnames_tag_h__
#define	__attnames_tag_h__

#ifdef __SMURPH__

// Attribute name conversion

#define	ui_ibuf		_dac (NodeTag, ui_ibuf)
#define	ui_obuf		_dac (NodeTag, ui_obuf)
#define	cmd_line	_dac (NodeTag, cmd_line)
#define	host_id		_dac (NodeTag, host_id)
#define	pong_params	_dac (NodeTag, pong_params)
#define	app_flags	_dac (NodeTag, app_flags)
#define sens_data	_dac (NodeTag, sens_data)
#define lh_time		_dac (NodeTag, lh_time)
#define ref_time	_dac (NodeTag, ref_time)
#define sens_dump	_dac (NodeTag, sens_dump)
#define ref_clock	_dac (NodeTag, ref_clock)

// Methods
#define next_col_time		_dac (NodeTag, next_col_time)
#define show_ifla		_dac (NodeTag, show_ifla)
#define read_ifla		_dac (NodeTag, read_ifla)
#define save_ifla		_dac (NodeTag, save_ifla)
#define	stats			_dac (NodeTag, stats)
#define	app_diag		_dac (NodeTag, app_diag)
#define	net_diag		_dac (NodeTag, net_diag)
#define	get_mem			_dac (NodeTag, get_mem)
#define	msg_getTag_in		_dac (NodeTag, msg_getTag_in)
#define	msg_setTag_in		_dac (NodeTag, msg_setTag_in)
#define	msg_getTagAck_out	_dac (NodeTag, msg_getTagAck_out)
#define	msg_setTagAck_out	_dac (NodeTag, msg_setTagAck_out)
#define msg_pongAck_in		_dac (NodeTag, msg_pongAck_in)
#define	max_pwr			_dac (NodeTag, max_pwr)
#define	send_msg		_dac (NodeTag, send_msg)
#define	process_incoming	_dac (NodeTag, process_incoming)
#define fatal_err		_dac (NodeTag, fatal_err)
#define sens_init		_dac (NodeTag, sens_init)
#define init			_dac (NodeTag, init)
#define r_a_d			_dac (NodeTag, r_a_d)
#define wall_time		_dac (NodeTag, wall_time)
#define upd_on_ack		_dac (NodeTag, upd_on_ack)
#define handle_c_flags		_dac (NodeTag, handle_c_flags)
#define tmpcrap			_dac (NodeTag, tmpcrap)

#else	/* PICOS */

#include "attribs_tag.h"
#include "tarp.h"

#endif	/* SMURPH or PICOS */

#endif
