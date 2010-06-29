#ifndef	__attnames_peg_h__
#define	__attnames_peg_h__

#ifdef __SMURPH__

// Attribute name conversion

#define	ui_ibuf		_dac (NodePeg, ui_ibuf)
#define	ui_obuf		_dac (NodePeg, ui_obuf)
#define	cmd_line	_dac (NodePeg, cmd_line)

#define	host_id		_dac (NodePeg, host_id)
#define	sattr		_dac (NodePeg, sattr)
#define	touts		_dac (NodePeg, touts)
#define sstate		_dac (NodePeg, sstate)
#define	app_flags	_dac (NodePeg, app_flags)
#define oss_stack	_dac (NodePeg, oss_stack)
#define cters		_dac (NodePeg, cters)

// Methods
#define	stats			_dac (NodePeg, stats)
#define	app_diag		_dac (NodePeg, app_diag)
#define	net_diag		_dac (NodePeg, net_diag)
#define show_stuff		_dac (NodePeg, show_stuff)

#define	process_incoming	_dac (NodePeg, process_incoming)
#define	check_msg_size		_dac (NodePeg, check_msg_size)
#define	check_tag		_dac (NodePeg, check_tag)
#define	get_mem			_dac (NodePeg, get_mem)
#define	init			_dac (NodePeg, init)
#define next_cyc		_dac (NodePeg, next_cyc)
#define set_rf			_dac (NodePeg, set_rf)
#define set_state		_dac (NodePeg, set_state)

#define	msg_master_in		_dac (NodePeg, msg_master_in)
#define	msg_master_out		_dac (NodePeg, msg_master_out)
#define	msg_ping_in		_dac (NodePeg, msg_ping_in)
#define	msg_ping_out		_dac (NodePeg, msg_ping_out)
#define	msg_stats_in		_dac (NodePeg, msg_stats_in)
#define	msg_stats_out		_dac (NodePeg, msg_stats_out)
#define msg_sil_out		_dac (NodePeg, msg_sil_out)
#define msg_sil_in		_dac (NodePeg, msg_sil_in)
#define msg_cmd_out		_dac (NodePeg, msg_cmd_out)
#define msg_cmd_in		_dac (NodePeg, msg_cmd_in)

#define oss_ping_out		_dac (NodePeg, oss_ping_out)

#define	send_msg		_dac (NodePeg, send_msg)
#define strncpy			_dac (NodePeg, strncpy)
#else	/* PICOS */

#include "attribs_peg.h"
#include "tarp.h"

#endif	/* SMURPH or PICOS */

#endif
