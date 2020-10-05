/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#ifndef __eep_esn_h
#define __eep_esn_h

#define ESN_SIZE	1008
#define SVEC_SIZE	(ESN_SIZE / 16)
#define ESN_BSIZE	(EE_PAGE_SIZE / 4)
#define ESN_OSET	1
#define ESN_PACK	5

// IMPORTANT: always keep them away from the ESN's space
#define NVM_BOOT_LEN	10
#define EE_NID		0
#define EE_LH		(EE_NID + 2)
#define EE_MID		(EE_NID + 4)
// EE_APP: b0-b2, b3: encr; b4: binder; b5-b7 spare, b8-b15: tarp_ctrl.param
#define EE_APP		(EE_NID + 6)
#define EE_SENSRX_VER	(EE_NID + 8)
#endif
