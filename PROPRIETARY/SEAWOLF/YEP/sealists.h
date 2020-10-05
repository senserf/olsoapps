/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#ifndef __pg_sealists_h__
#define __pg_sealists_h__

#include "oep.h"
#include "lcdg_images.h"
#include "lcdg_dispman.h"

//+++ "sealists.c"

#include "sealists_types.h"

lcdg_dm_obj_t *seal_mkcmenu (Boolean);
lcdg_dm_obj_t *seal_mkrmenu ();
sea_rec_t *seal_getrec (word);
word seal_findrec (lword);
void seal_disprec (sea_rec_t*, Boolean);

#define seal_objaddr(ptr)	((lword)((ptr)+SEA_EOFF_TXT))
#define seal_gettext(off) 	lcdg_dm_newtext_e (seal_objaddr (off))

#endif
