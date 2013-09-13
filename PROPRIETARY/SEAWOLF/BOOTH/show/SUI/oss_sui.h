#ifndef __oss_sui_h
#define __oss_sui_h
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2013.                   */
/* All rights reserved.                                                 */
/* ==================================================================== */

//+++ oss_sui.cc

#include "app_sui.h"

void 	oss_profi_out (word ind, word list);
void	oss_data_out (word ind);
void 	oss_alrm_out (char * buf);
void	oss_nvm_out (nvmDataType * buf, word slot);

#endif
