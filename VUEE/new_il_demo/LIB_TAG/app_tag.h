#ifndef __app_tag_h__
#define __app_tag_h__
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2010                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "sysio.h"
#include "msg_tarp.h"
#include "storage.h"

#define EE_SENS_SIZE	sizeof(sensEEDataType)
#define EE_SENS_MAX	(lword)(ee_size (NULL, NULL) / EE_SENS_SIZE -1)
#define EE_SENS_MIN	0L
//test: #define EE_SENS_MIN (EE_SENS_MAX - 4)

// safeguard to kill hanging collection (in ms)
#define SENS_COLL_TIME	10
#define NUM_SENS	6

#define SENS_FF		0xF
#define SENS_IN_USE	0xC
#define SENS_COLLECTED	0x8
#define SENS_CONFIRMED	0x0
// plot marker or a wildcard for reporting only
#define SENS_ALL	0xE

// mark is :3 
#define MARK_FF         7 
#define MARK_BOOT       6
#define MARK_PLOT       5
#define MARK_SYNC       4
#define MARK_FREQ       3
#define MARK_DATE       2

#define ERR_EER		0xFFFE
#define ERR_SLOT	0xFFFC
#define ERR_FULL	0xFFF8
#define ERR_MAINT	0xFFF0

#define LED_R   0
#define LED_G   1
#define LED_B   2

#define LED_OFF 0
#define LED_ON  1
#define LED_BLINK 2

#define SIY     (365L * 24 * 60 * 60)
#define SID     (24L * 60 * 60)
#define TIME_TOLER	2

// trigger / when ids
#define TRIGGER_BASE_ID	77

// command line
#define CMD_READER	(TRIGGER_BASE_ID +0)
#define CMD_WRITER	(TRIGGER_BASE_ID +1)

#define SENS_DONE	(TRIGGER_BASE_ID +2)
#define OSS_DONE	(TRIGGER_BASE_ID +3)

// rx switch control
#define RX_SW_ON	(TRIGGER_BASE_ID +4)

// for pongAcks
#define ACK_IN		(TRIGGER_BASE_ID +5)

#define ALRMS		(TRIGGER_BASE_ID +6)

// end of trigger / when ids

typedef union {
        lint secs;
        struct {
                word f  :1;
                word yy :5;
                word dd :5;
                word h  :5; // just in case, no word crossing
                word mm :4;
                word m  :6;
                word s  :6;
        } dat;
} mdate_t;

typedef union {
		word b :8;
		struct {
			word emptym :1;
			word mark   :3;
			word status :4;
	} f;
	word spare :8;
} statu_t;

typedef struct pongParamsStruct {
	word freq_maj;
	word freq_min;
	word pow_levels;
	word rx_span;
	word rx_lev:8;
	word pload_lev:8;
} pongParamsType;

typedef struct sensEEDataStruct {
	statu_t	s; // keep it bytes 0,1 of the ee slot
	word sval [NUM_SENS];
	lint ds;
} sensEEDataType; // 6 + NUM_SENS * 2 (+2 if misaligned)

typedef struct sensEEDumpStruct {
	sensEEDataType ee;
	lword   fr;
	lword   to;
	lword   ind;
	lword   cnt;
	word    upto;
	statu_t s;
	word    dfin   :1;
	word	spare  :7;
} sensEEDumpType;

typedef struct sensDataStruct {
	sensEEDataType ee;
	lword eslot;
} sensDataType;

/* app_flags definition [default]:
   bit 0: synced (unique to tags) [0]
   bit 1: master changed (in TARP) [0]
   bit 2: ee write collected [0]
   bit 3: ee write confirmed [0]
   bit 4: ee overwrite (cyclic stack) [0]
   bit 5:  alrms on (being reported at the moment) [0]

   let's leave bits 0-7 for praxis controls and use the 2nd byte for alarms
   bit 8:  alrm0 (motion on WARSAW_ILS) [0]
   bit 9:  alrm1 (light on WARSAW_ILS) [0]
*/
#define DEF_APP_FLAGS   0

#define set_synced	(app_flags |= 1)
#define clr_synced	(app_flags &= ~1)
#define is_synced	(app_flags & 1)

// master_chg not used, but needed for TARP

#define set_eew_coll    (app_flags |= 4)
#define clr_eew_coll    (app_flags &= ~4)
#define is_eew_coll     (app_flags & 4)

#define set_eew_conf    (app_flags |= 8)
#define clr_eew_conf    (app_flags &= ~8)
#define is_eew_conf     (app_flags & 8)

#define set_eew_over    (app_flags |= 16)
#define clr_eew_over    (app_flags &= ~16)
#define is_eew_over     (app_flags & 16)

#define set_alrms	(app_flags |= 32)
#define clr_alrms	(app_flags &= ~32)
#define is_alrms	(app_flags & 32)

#define set_alrm0	(app_flags |= 256)
#define clr_alrm0	(app_flags &= ~256)
#define is_alrm0	(app_flags & 256)

#define set_alrm1	(app_flags |= 512)
#define clr_alrm1	(app_flags &= ~512)
#define is_alrm1	(app_flags & 512)

#endif
