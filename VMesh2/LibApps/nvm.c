#include "nvm.h"
#include "app.h"
#include "app_tarp_if.h"
#include "codes.h"
#include "msg_vmesh.h"
#include "lib_app_if.h"
#include "pinopts.h"
#include "tarp.h"

/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2006.                   */
/* All rights reserved.                                                 */
/* ==================================================================== */

#define nvm_erase(a)	do { \
				if_erase(a); \
				dbg_f(0xF000 | (a)); \
			} while (0)

void nvm_read (word pos, address d, word wlen) {
	if (wlen == 0 || pos + wlen > IFLASH_SIZE) {
		dbg_2 (0xE000 | (byte)wlen);
		return;
	}
	while (wlen--)
		*d++ = IFLASH [pos++];
}

extern tarpCtrlType tarp_ctrl;
static void fpage_reset (word p) {
	lword lw;
	word w, w1;

	nvm_erase (p);
	if (p & 0xFFC0) { // counter page
		// clear b7, b3, b2
		lw = pmon_get_cnt() << 8 | (pmon_get_state() & 0x73);
		if_write (NVM_IO_STATE, (word)lw);
		if_write (NVM_IO_STATE +1, (word)(lw >> 16));
	} else {
		if_write (NVM_NID, net_id);
		if_write (NVM_LH, local_host);
		if_write (NVM_MID, master_host);
		w = encr_data;
		if (is_cmdmode)
			w |= 1 << 5;
		if (is_binder)
			w |= 1 << 4;
		w |= tarp_ctrl.param << 8;
		if_write (NVM_APP, w);
		ion (UART, CONTROL, (char*) &w, UART_CNTRL_GETRATE);
		if_write (NVM_UART, w);
		memcpy (&w, &cyc_ctrl, 2);
		if_write (NVM_CYC_CTRL, w);
		if_write (NVM_CYC_SP, (word)cyc_sp);
		if_write (NVM_CYC_SP +1, (word)(cyc_sp >> 16));
		lw = pmon_get_cmp();
		if_write (NVM_IO_CMP, (word)lw);
		if_write (NVM_IO_CMP +1, (word)(lw >> 16));
		if_write (NVM_IO_CREG, (word)io_creg);
		if_write (NVM_IO_CREG +1, (word)(io_creg >> 16));
		lw = 0;
		for (w1 = 1; w1 < 11; w1++) {
			w = pin_read (w1);
			switch (w1) {
				case 1:
				case 2:
				case 3:
				case 8:
				case 9:
				case 10:
					if (w == 2 || w == 3)
						w -= 2;
					else if (w != 4)
						w = 7;
					break;
				case 4:
				case 5:
					if (w == 0 || w == 1)
						w = 2;
					else if (w != 4)
						w = 7;
					break;
				case 6:
				case 7:
					if (w == 0 || w == 1)
						w = 2;
					else if (w == 2 || w == 3)
						w -= 2;
					else w = 7;
					break;
				default:
					w = 7;
			}
			lw |= (lword)w << 3 * (w1 -1);
		}
		if_write (NVM_IO_PINS, (word)lw);
		if_write (NVM_IO_PINS +1, (word)(lw >> 16));
	}
}

void nvm_write (word pos, const word * s, word wlen) {
	if (wlen == 0 || pos + wlen >= IFLASH_SIZE) {
		dbg_2 (0xE100 | (byte)wlen);
		return;
	}

	while (wlen--) {
		if (IFLASH [pos] == *s) {
			pos++; s++;
			continue;
		}
		if (if_write (pos, *s) != 0) {
			fpage_reset (pos);
			return;
		} else {
			 pos++; s++;
		}
	}
}

void app_reset (word lev) {
	if (lev)
		nvm_erase(-1);
	reset();
}

void nvm_io_backup () {
	lword lw = pmon_get_cnt() << 8 | (pmon_get_state() & 0x73);
	word i = NVM_IO_STATE;
	while (IFLASH [i] != 0xFFFF || IFLASH [i+1] != 0xFFFF)
		if ((i += 2) >= (NVM_PAGE_SIZE << 1) -1) {
			if (IFLASH [i -2] != (word)lw ||
					IFLASH [i -1] != (lw >> 16))
				fpage_reset (NVM_PAGE_SIZE);
			return;
		}
	if (i > NVM_IO_STATE && IFLASH [i -2] == (word)lw &&
			IFLASH [i -1] == (lw >> 16))
		return;
	if (if_write (i, (word)lw) == -1 ||
		if_write (i +1, (word)(lw >> 16)) == -1) {
			dbg_2 (0xE200 | (byte)i);
			fpage_reset (NVM_PAGE_SIZE);
			return;
	}
}

