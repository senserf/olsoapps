#ifndef	__attnames_tag_h__
#define	__attnames_tag_h__

#ifdef __SMURPH__

// Attribute name conversion

#define	ui_ibuf		_dac (NodeTag, ui_ibuf)
#define	ui_obuf		_dac (NodeTag, ui_obuf)
#define	cmd_line	_dac (NodeTag, cmd_line)
#define	host_id		_dac (NodeTag, host_id)
#define	host_passwd	_dac (NodeTag, host_passwd)
#define	app_count	_dac (NodeTag, app_count)
#define	pong_params	_dac (NodeTag, pong_params)
#define	app_flags	_dac (NodeTag, app_flags)

// Methods
#define	stats			_dac (NodeTag, stats)
#define	app_diag		_dac (NodeTag, app_diag)
#define	net_diag		_dac (NodeTag, net_diag)
#define	check_passwd		_dac (NodeTag, check_passwd)
#define	get_mem			_dac (NodeTag, get_mem)
#define	set_tag			_dac (NodeTag, set_tag)
#define	msg_getTag_in		_dac (NodeTag, msg_getTag_in)
#define	msg_setTag_in		_dac (NodeTag, msg_setTag_in)
#define	msg_getTagAck_out	_dac (NodeTag, msg_getTagAck_out)
#define	msg_setTagAck_out	_dac (NodeTag, msg_setTagAck_out)
#define	max_pwr			_dac (NodeTag, max_pwr)
#define	send_msg		_dac (NodeTag, send_msg)
#define	process_incoming	_dac (NodeTag, process_incoming)

#else	/* PICOS */

#include "attribs_tag.h"
#include "tarp.h"

// These ones do not belong to the application, so they must be explicitly
// mentioned as externals (for SMURPH, they come from stdattr.h).

extern 	tarpCtrlType	tarp_ctrl;
extern	nid_t		net_id; 
extern	nid_t		local_host;
extern	nid_t   	master_host;

#endif	/* SMURPH or PICOS */

#endif
