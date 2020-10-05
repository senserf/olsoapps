/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#ifndef	__app_lcd_data_h
#define	__app_lcd_data_h

#include "app_lcd.h"
#include "msg_tarp.h"

extern word host_pl;

extern profi_t profi_att, p_inc, p_exc;
extern char desc_att [PEG_STR_LEN +1];
extern char d_biz [PEG_STR_LEN +1];
extern char d_priv [PEG_STR_LEN +1];
extern char d_alrm [PEG_STR_LEN +1];
extern char nick_att [NI_LEN +1];

void 	msg_profi_out (nid_t peg);
void	msg_alrm_out (nid_t peg, word level, char * desc);

#endif
