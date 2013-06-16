#ifndef __sensets_h
#define __sensets_h
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2012                           */
/* All rights reserved.                                                 */
/* ==================================================================== */

/* note that we condense and circumvent some board types and sensors.
   E.g., we consider PAR, PYR useless and leave out moisture sensors.
   Of course, it is easy to change.
   We load these into PREINITed const senset, to allow sensor selection, VUEE,
   and OSSI snippets process the readings.

   All this proves to be different but as bad as the other attempts:
   separate simps and arras programs are not needed. Will be rewritten ;-)
*/

#define SENSET_GENERIC	0
#define SENSET_ECO_12	1
#define SENSET_ECO_45	2
#define SENSET_ILS	3
#define SENSET_CHRO	4
#define SENSET_10SHT	5
#define SENSET_SONAR	6

//+++ "sensets.cc"

#endif
