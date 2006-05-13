#ifndef __eep_esn_h
#define __eep_esn_h
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2005.                   */
/* All rights reserved.                                                 */
/* ==================================================================== */
// #include "sysio.h"

#define ESN_SIZE	1008
#define SVEC_SIZE	(ESN_SIZE / 16)
#define ESN_BSIZE	(EE_PAGE_SIZE / 4)
#define ESN_OSET	1
#define ESN_PACK	5

// IMPORTANT: always keep them away from the ESN's space
#define EE_NID		0
// this MUST be together
#define EE_LH		(EE_NID + 2)
#define EE_MID		(EE_NID + 4)
#endif
