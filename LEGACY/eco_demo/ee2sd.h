/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#ifndef __ee2sd_h__
#define __ee2sd_h__

#ifndef __SMURPH__
#if STORAGE_SDCARD
#define ee_open                 sd_open
#define ee_close                sd_close
#define ee_size                 sd_size
#define ee_read                 sd_read
#define ee_sync(a)              sd_sync()
#define ee_erase(a,b,c)		sd_erase(b,c)
#define ee_write(a,b,c,d)       sd_write(b,c,d)
#endif
#endif

#endif
