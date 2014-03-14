#ifndef __vartypes_h__
#define __vartypes_h__
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2014.                          */
/* All rights reserved.                                                 */
/* ==================================================================== */
#include "sysio.h"
/* Board- and pgm- specific */

// these pongPloadType. must be accessible to decode payloads
typedef struct pongPloadStruct0 {
	word	volt;
	word	move_ago;
	word	move_nr;
} pongPloadType0;

typedef struct pongPloadStruct1 {
	word	volt;
	word	move_ago;
	word	move_nr;
} pongPloadType1;

typedef struct pongPloadStruct2 {
	word	volt;
} pongPloadType2;

typedef struct pongPloadStruct3 {
	word	volt;
	word	dial; // 2 dials 0..99, best to handle as hex(?)
} pongPloadType3;

typedef struct pongPloadStruct4 {
	word	volt;
} pongPloadType4;

typedef struct pongPloadStruct5 {
	word	volt;
	word	random_shit;
	word	steady_shit;
} pongPloadType5;

#define PTYPE_PEG	0
#define PTYPE_TAG	1

#define BTYPE_CHRONOS				0
#define BTYPE_CHRONOS_WHITE			1
#define BTYPE_ALPHATRONICS_BASE		2
#define BTYPE_ALPHATRONICS_BUTTON	3
#define BTYPE_ALPHATRONICS_PANIC	4
#define BTYPE_WARSAW				5

#ifdef PGMLABEL_peg
#define PTYPE	PTYPE_PEG
#elif defined PGMLABEL_chrt || defined PGMLABEL_wart
#define PTYPE	PTYPE_TAG
#else
#error PTYPE?
error PTYPE FIXME
#endif

#ifdef BOARD_CHRONOS
#define BTYPE	BTYPE_CHRONOS
typedef pongPloadType0 pongPloadType;
#endif

#ifdef BOARD_CHRONOS_WHITE
#define BTYPE   BTYPE_CHRONOS_WHITE
typedef pongPloadType1 pongPloadType;
#endif

#ifdef BOARD_ALPHATRONICS_BASE
#define BTYPE   BTYPE_ALPHATRONICS_BASE
typedef pongPloadType2 pongPloadType;
#endif

#ifdef BOARD_ALPHATRONICS_BUTTON
#define BTYPE   BTYPE_ALPHATRONICS_BUTTON
typedef pongPloadType3 pongPloadType;
#endif

#ifdef BOARD_ALPHATRONICS_PANIC
#define BTYPE   BTYPE_ALPHATRONICS_PANIC
typedef pongPloadType4 pongPloadType;
#endif

#if defined BOARD_WARSAW || defined BOARD_WARSAW_BLUE
#define BTYPE   BTYPE_WARSAW
typedef pongPloadType5 pongPloadType;
#endif

#ifndef BTYPE
#error unsupported board
error unsupported board FIXME
#endif

// likely not needed... free, anyway
#define NTYPE ((BTYPE << 4) | PTYPE)

#endif
