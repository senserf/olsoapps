#ifndef	__attnames_peg_h__
#define	__attnames_peg_h__

#ifdef __SMURPH__

// Attribute name conversion

#define	ui_ibuf		_dac (NodePeg, ui_ibuf)
#define	ui_obuf		_dac (NodePeg, ui_obuf)
#define	cmd_line	_dac (NodePeg, cmd_line)

#define	host_id		_dac (NodePeg, host_id)
#define	host_passwd	_dac (NodePeg, host_passwd)
#define	host_pl		_dac (NodePeg, host_pl)
#define	app_count	_dac (NodePeg, app_count)
#define	master_delta	_dac (NodePeg, master_delta)
#define	msg4tag		_dac (NodePeg, msg4tag)
#define	msg4ward	_dac (NodePeg, msg4ward)
#define	tagArray	_dac (NodePeg, tagArray)
#define	tag_auditFreq	_dac (NodePeg, tag_auditFreq)
#define	tag_eventGran	_dac (NodePeg, tag_eventGran)
#define	app_flags	_dac (NodePeg, app_flags)

// Methods
#define	stats			_dac (NodePeg, stats)
#define	app_diag		_dac (NodePeg, app_diag)
#define	net_diag		_dac (NodePeg, net_diag)

#define	countTags		_dac (NodePeg, countTags)
#define	process_incoming	_dac (NodePeg, process_incoming)
#define	check_msg4tag		_dac (NodePeg, check_msg4tag)
#define	check_msg_size		_dac (NodePeg, check_msg_size)
#define	check_tag		_dac (NodePeg, check_tag)
#define	find_tag		_dac (NodePeg, find_tag)
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
#define	msg_getTagAck_in	_dac (NodePeg, msg_getTagAck_in)
#define	msg_setTagAck_in	_dac (NodePeg, msg_setTagAck_in)
#define	msg_report_out		_dac (NodePeg, msg_report_out)
#define	msg_fwd_in		_dac (NodePeg, msg_fwd_in)
#define	msg_fwd_out		_dac (NodePeg, msg_fwd_out)
#define	msg_fwd_msg		_dac (NodePeg, msg_fwd_msg)
#define	copy_fwd_msg		_dac (NodePeg, copy_fwd_msg)

#define	oss_findTag_in		_dac (NodePeg, oss_findTag_in)
#define	oss_getTag_in		_dac (NodePeg, oss_getTag_in)
#define	oss_setTag_in		_dac (NodePeg, oss_setTag_in)
#define	oss_setPeg_in		_dac (NodePeg, oss_setPeg_in)
#define	oss_master_in		_dac (NodePeg, oss_master_in)
#define	oss_report_out		_dac (NodePeg, oss_report_out)
#define	oss_getTag_out		_dac (NodePeg, oss_getTag_out)
#define	oss_setTag_out		_dac (NodePeg, oss_setTag_out)

#define	send_msg		_dac (NodePeg, send_msg)

#else	/* PICOS */

#include "attribs_peg.h"
#include "tarp.h"

// These ones do not belong to the application, so they must be explicitly
// mentioned as externals (for SMURPH, they come from stdattr.h).

extern 	tarpCtrlType	tarp_ctrl;
extern	nid_t		net_id; 
extern	nid_t		local_host;
extern	nid_t   	master_host;

#endif	/* SMURPH or PICOS */

#endif
