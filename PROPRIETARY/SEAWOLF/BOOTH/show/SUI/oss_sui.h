/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#ifndef __oss_sui_h
#define __oss_sui_h

//+++ oss_sui.cc

#include "app_sui.h"

void	oss_over_profi_out (char * buf, word rssi);
void 	oss_profi_out (word ind, word list);
void	oss_data_out (word ind);
void 	oss_alrm_out (char * buf);
void	oss_nvm_out (nvmDataType * buf, word slot);

#endif
