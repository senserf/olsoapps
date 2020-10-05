/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#ifndef __diag_h__
#define __diag_h__

// this here compared with mm saves 1882B for D_OFF (presumptions made in
// mm for ..._diag constructs are false, although I thought I tested the crap
// many moons ago.

#include "sysio.h"

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
#define APP_DL		D_WARNING
#define NET_DL		D_WARNING

// compile out what's not needed
#if NET_DL < D_DEBUG
#define net_diag_D(...) CNOP
#else
#define net_diag_D(...) _net_diag(__VA_ARGS__)
#endif

#if APP_DL < D_DEBUG
#define app_diag_D(...) CNOP
#else
#define app_diag_D(...) _app_diag(__VA_ARGS__)
#endif

#if NET_DL < D_INFO
#define net_diag_I(...) CNOP
#else
#define net_diag_I(...) _net_diag(__VA_ARGS__)
#endif

#if APP_DL < D_INFO
#define app_diag_I(...) CNOP
#else
#define app_diag_I(...) _app_diag(__VA_ARGS__)
#endif

#if NET_DL < D_WARNING
#define net_diag_W(...) CNOP
#else
#define net_diag_W(...) _net_diag(__VA_ARGS__)
#endif

#if APP_DL < D_WARNING
#define app_diag_W(...) CNOP
#else
#define app_diag_W(...) _app_diag(__VA_ARGS__)
#endif

#if NET_DL < D_SERIOUS
#define net_diag_S(...) CNOP
#else
#define net_diag_S(...) _net_diag(__VA_ARGS__)
#endif

#if APP_DL < D_SERIOUS
#define app_diag_S(...) CNOP
#else
#define app_diag_S(...) _app_diag(__VA_ARGS__)
#endif

#if NET_DL < D_FATAL
#define net_diag_F(...) CNOP
#else
#define net_diag_F(...) _net_diag(__VA_ARGS__)
#endif

#if APP_DL < D_FATAL
#define app_diag_F(...) CNOP
#else
#define app_diag_F(...) _app_diag(__VA_ARGS__)
#endif

#if NET_DL < D_UI
#define net_diag_U(...) CNOP
#else
#define net_diag_U(...) _net_diag(__VA_ARGS__)
#endif

#if APP_DL < D_UI
#define app_diag_U(...) CNOP
#else
#define app_diag_U(...) _app_diag(__VA_ARGS__)
#endif

void  _app_diag (const char *, ...);
void  _net_diag (const char *, ...);

//+++ diag.cc
#endif
