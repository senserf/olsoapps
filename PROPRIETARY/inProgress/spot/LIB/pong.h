/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#ifndef __pong_h__
#define __pong_h__

#include "commons.h"

extern pongParamsType  pong_params;

extern char pong_frame [sizeof(msgPongType) + sizeof(pongPloadType)];
extern fifek_t pframe_stash;

void init_pframe ();
void load_pframe ();
void stash_pframe ();
void upd_pframe (word pl, word tnr); // signature may end up board-specific(?)

fsm pong;

//+++ pong.cc

#endif
