#ifndef __ee2sd_h__
#define __ee2sd_h__
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2009.                   */
/* All rights reserved.                                                 */
/* ==================================================================== */

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
