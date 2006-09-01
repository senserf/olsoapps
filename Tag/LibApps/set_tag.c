/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2004.                   */
/* All rights reserved.                                                 */
/* ==================================================================== */
#include "sysio.h"
#include "msg_tags.h"
#include "lib_apps.h"

extern lword host_id;
extern nid_t local_host;
extern lword host_passwd;

void set_tag (char * buf) {
	// we may need more scrutiny...
	if (in_setTag(buf, node_addr) != 0)
		local_host = in_setTag(buf, node_addr);
	if (in_setTag(buf, pow_levels) != 0)
		pong_params.pow_levels = in_setTag(buf, pow_levels);
	if (in_setTag(buf, freq_maj) != 0)
		pong_params.freq_maj = in_setTag(buf, freq_maj);
	if (in_setTag(buf, freq_min) != 0)
		pong_params.freq_min = in_setTag(buf, freq_min);
	if (in_setTag(buf, rx_span) != 0)
		pong_params.rx_span = in_setTag(buf, rx_span);
	if (in_setTag(buf, npasswd) != 0)
		host_passwd = in_setTag(buf, npasswd);
}
