#ifndef __variants_h__
#define __variants_h__
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2014.                          */
/* All rights reserved.                                                 */
/* ==================================================================== */

/* Board-specifics (tag? peg?) */

#include "commons.h"

// these must be accessible on the master(?)
typedef struct pongPloadStruct0 {
        word    volt;
        word    move_ago;
        word    move_nr;
} pongPloadType0;

typedef struct pongPloadStruct1 {
        word    volt;
        word    move_ago;
        word    move_nr;
} pongPloadType1;

typedef struct pongPloadStruct2 {
        word    volt;
} pongPloadType2;

typedef struct pongPloadStruct3 {
        word    volt;
        word    dial; // 2 dials 0..99, best to handle as hex(?)
} pongPloadType3;

typedef struct pongPloadStruct4 {
        word    volt;
} pongPloadType4;

typedef struct pongPloadStruct5 {
        word    volt;
        word    random_shit;
        word    steady_shit;
} pongPloadType5;

#ifdef BOARD_CHRONOS
#define BTYPE	0;
typedef pongPloadType0 pongPloadType;
#endif

#ifdef BOARD_CHRONOS_WHITE
#define BTYPE   1;
typedef pongPloadType1 pongPloadType;
#endif

#ifdef BOARD_ALPHATRONICS_BASE
#define BTYPE   2;
typedef pongPloadType2 pongPloadType;
#endif

#ifdef BOARD_ALPHATRONICS_BUTTON
#define BTYPE   3;
typedef pongPloadType3 pongPloadType;
#endif

#ifdef BOARD_ALPHATRONICS_PANIC
#define BTYPE   4;
typedef pongPloadType4 pongPloadType;
#endif

#ifdef BOARD_WARSAW
#define BTYPE   5;
typedef pongPloadType5 pongPloadType;
#endif

#ifndef BTYPE
#error unsupported board
error unsupported board FIXME
#endif

// not for pegs
#ifndef PGMLABEL_peg
extern char pong_frame [sizeof(msgPongType) + sizeof(pongPloadType)];

void init_pframe ();
void load_pframe ();
void upd_pframe (word pl, word tnr); // signature may end up board-specific

void set_alrm (word a);
void clr_alrm ();

//+++ variants.cc

#endif

#endif
