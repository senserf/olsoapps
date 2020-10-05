/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#ifndef __diag_h
#define __diag_h

// level defs
#define D_OFF		0
#define D_UI		2
#define D_FATAL		4
#define D_SERIOUS	6
#define D_WARNING	8
#define D_INFO		10
#define D_DEBUG		12
#define D_ALL		42

// current levels
//#define net_dl		D_DEBUG
//#define app_dl		D_DEBUG
#define net_dl          D_WARNING
#define app_dl          D_WARNING

//+++ "net_diag.c" "app_diag.c"

void net_diag (const word, const char *, ...);
void app_diag (const word, const char *, ...);



#endif
