/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#ifndef __codes_h
#define __codes_h

// CHECK app.c::valid_input() when changing the codes
#define CMD_GET 	0x01
#define CMD_SET		0x02
#define CMD_MASTER	0x03
#define CMD_TRACE	0x04
#define CMD_BIND	0x05
#define CMD_SACK	0x06
#define CMD_SNACK_OBSOLETE	0x07
#define CMD_SENS_OBSOLETE	0x08
#define CMD_RESET	0x09
#define CMD_LOCALE	0x0A
#define CMD_IO		0x0B
#define CMD_IOACK	0x0C
#define CMD_DAT		0x0D
#define CMD_DATACK	0x0E

#define CMD_MSGOUT	0xFF
#define CMD_SENSIN	0xFE
#define CMD_DIAG	0xFD

#define RC_NONE		0xFF
#define RC_OK		0
#define RC_OKRET	1
#define RC_ERES		2
#define RC_ENET		3
#define RC_EMAS		4
#define RC_ECMD		5
#define RC_EPAR		6
#define RC_EVAL		7
#define RC_ELEN		8
#define RC_EADDR	9
#define RC_EFORK	10
#define RC_EMEM		11

#define PAR_ERR 	0
#define PAR_LH		1
#define ATTR_ESN	2
#define ATTR_MID	3
#define PAR_NID		4
#define PAR_TARP_L	5
#define PAR_TARP_S	6
#define PAR_TARP_R	7
#define PAR_TARP_F	8
#define PAR_BEAC	9
#define PAR_CON		10
#define PAR_RF		11
#define PAR_ALRM_DONT	12
#define PAR_STAT_REP	13
#define PAR_RETRY	14
#define PAR_AUTOACK	15
#define ATTR_UPTIME	16
#define ATTR_BRIDGE_OBSOLETE	17
#define ATTR_RSSI	18
#define ATTR_MSG_COUNT_T 19
#define PAR_RFPWR_OBSOLETE	20
#define ATTR_MEM1	21
#define ATTR_MEM2	22
#define PAR_ENCR_M	23
#define PAR_ENCR_K	24
#define PAR_ENCR_D	25
#define ATTR_SYSVER	26
#define ATTR_NHOOD	27
#define PAR_BINDER	28
#define PAR_UART	29
#define PAR_CYC_CTRL	30
#define PAR_CYC_SLEEP	31
#define PAR_CYC_M_SYNC	32
#define PAR_CYC_M_REST	33
#define ATTR_CYC_LEFT	34
#define PAR_CMD_MODE	35
// racing genesis - vmesh leaves 36 unused
#define PAR_RSSI_THOLD	37
#define PAR_TARP_D	38
#endif
