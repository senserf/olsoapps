#ifndef	__attnames_peg_h__
#define	__attnames_peg_h__

#ifdef __SMURPH__

// Attribute name conversion

#define	ui_ibuf		_dac (NodePeg, ui_ibuf)
#define	ui_obuf		_dac (NodePeg, ui_obuf)
#define	cmd_line	_dac (NodePeg, cmd_line)

#define	host_id		_dac (NodePeg, host_id)
#define	host_pl		_dac (NodePeg, host_pl)
#define	tagArray	_dac (NodePeg, tagArray)
#define ignArray	_dac (NodePeg, ignArray)
#define monArray	_dac (NodePeg, monArray)
#define	tag_auditFreq	_dac (NodePeg, tag_auditFreq)
#define	tag_eventGran	_dac (NodePeg, tag_eventGran)
#define	app_flags	_dac (NodePeg, app_flags)
#define profi_att	_dac (NodePeg, profi_att)
#define p_inc		_dac (NodePeg, p_inc)
#define p_exc		_dac (NodePeg, p_exc)
#define d_biz		_dac (NodePeg, d_biz)
#define d_priv		_dac (NodePeg, d_priv)
#define d_alrm		_dac (NodePeg, d_alrm)
#define nick_att	_dac (NodePeg, nick_att)
#define desc_att	_dac (NodePeg, desc_att)
#define led_state	_dac (NodePeg, led_state)

// Methods
#define	stats			_dac (NodePeg, stats)
#define	app_diag		_dac (NodePeg, app_diag)
#define	net_diag		_dac (NodePeg, net_diag)

#define	process_incoming	_dac (NodePeg, process_incoming)
#define	check_msg_size		_dac (NodePeg, check_msg_size)
#define	check_tag		_dac (NodePeg, check_tag)
#define	find_tag		_dac (NodePeg, find_tag)
#define find_ign		_dac (NodePeg, find_ign)
#define find_mon		_dac (NodePeg, find_mon)
#define	get_mem			_dac (NodePeg, get_mem)
#define	init_tag		_dac (NodePeg, init_tag)
#define init_ign		_dac (NodePeg, init_ign)
#define init_mon		_dac (NodePeg, init_mon)
#define	init_tags		_dac (NodePeg, init_tags)
#define	insert_tag		_dac (NodePeg, insert_tag)
#define insert_ign		_dac (NodePeg, insert_ign)
#define insert_mon		_dac (NodePeg, insert_mon)
#define	set_tagState		_dac (NodePeg, set_tagState)

#define	msg_profi_in		_dac (NodePeg, msg_profi_in)
#define	msg_profi_out		_dac (NodePeg, msg_profi_out)
#define	msg_data_in		_dac (NodePeg, msg_data_in)
#define	msg_data_out		_dac (NodePeg, msg_data_out)
#define	msg_alrm_in		_dac (NodePeg, msg_alrm_in)
#define	msg_alrm_out		_dac (NodePeg, msg_alrm_out)

#define	oss_profi_out		_dac (NodePeg, oss_profi_out)
#define oss_alrm_out		_dac (NodePeg, oss_alrm_out)
#define oss_data_out		_dac (NodePeg, oss_data_out)

#define	send_msg		_dac (NodePeg, send_msg)
#define strncpy			_dac (NodePeg, strncpy)
#else	/* PICOS */

#include "attribs_peg.h"
#include "tarp.h"

#endif	/* SMURPH or PICOS */

#endif
