/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2004.                   */
/* All rights reserved.                                                 */
/* ==================================================================== */
#include "sysio.h"
#include "lib_apps.h"
#include "diag.h"
#include "msg_pegStructs.h"

void msg_master_in (char * buf) {

	master_delta = in_master(buf, mtime) - seconds();
	master_host  = in_header(buf, snd); // blindly, for now
	app_diag (D_INFO, "Set master to %u at %ld", master_host,
			master_delta);
}
