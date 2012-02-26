#include "sysio.h"
#include "sensets.h"

#ifdef __SMURPH__
const word senset = (word) PREINIT (0, "SSET");
#else

#ifdef BOARD_WARSAW_10SHT
const word senset = SENSET_10SHT;
// BOARD_WARSAW_10SHT: IT(-2) IV(-1) 4x (T, RH)(0, 1)
#else

#ifdef BOARD_WARSAW_ILS
// BOARD_WARSAW_ILS: IT(-2) IV(-1) LIGHT(0) IR(1)
const word senset = SENSET_ILS;
#else

#ifdef BOARD_WARSAW_IR250LP
// BOARD_WARSAW_IR250LP: IT(-2) IV(-1) LIGHT(0) IR(1)
const word senset = SENSET_ILS;
#else

#ifdef BOARD_ARTURO
// BOARD_ARTURO: selected IT(-2) IV(-1) T(1) RH(2)
const word senset = SENSET_ECO_12;
#else

#ifdef BOARD_ARTURO_PMTH
// BOARD_ARTURO_PMTH: selected IT(-2) IV(-1) T(1) RH(2)
const word senset = SENSET_ECO_12;
#else

#ifdef BOARD_ARTURO_PYR
// BOARD_ARTURO_PYR: selected IT(-2) IV(-1) T(4) RH(5)
const word senset = SENSET_ECO_45;
#else

#ifdef BOARD_CHRONOS
// BOARD_CHRONOS: IT(-2) IV(-1) CMA(0) T(1) (pressure not used)
const word senset = SENSET_CHRO;
#else
// pseudoBOARD_GENERIC: IT(-2) IV(-1)
const word senset = SENSET_GENERIC;
#endif
#endif
#endif
#endif
#endif
#endif
#endif
#endif

