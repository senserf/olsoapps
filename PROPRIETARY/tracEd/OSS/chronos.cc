/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2012                           */
/* All rights reserved.                                                 */
/* ==================================================================== */

// for now, a modified copy from CHRONOS/RFTEST
// perhaps we can have it lower and cheaper than form, later
#include "form.h"
#ifndef __SMURPH__
static void chro_lcd (const char *txt, word fr, word to) {
//
// Displays characters on the LCD
//
        char c;

        while (1) {
		if ((c = *txt) != '\0') {
			if (c >= 'a' && c <= 'z')
				c = c - 'a' + 10;
			else if (c >= 'A' && c <= 'Z')
				c = c - 'A' + 10;
			else if (c >= '0' && c <= '9')
				c -= '0';
			else
				c = 32;
                        ezlcd_item (fr, (word)c | LCD_MODE_SET);
                        txt++;
                } else {
                        ezlcd_item (fr, LCD_MODE_CLEAR);
                }
                if (fr == to)
                        return;
                if (fr > to)
                        fr--;
                else
                        fr++;
        }
}

void chro_hi (const char *txt) { chro_lcd (txt, LCD_SEG_L1_3, LCD_SEG_L1_0); }
void chro_lo (const char *txt) { chro_lcd (txt, LCD_SEG_L2_4, LCD_SEG_L2_0); }

void chro_nn (word hi, word a) {

        char erm [6];

        if (hi) {
                form (erm, "%u", a % 10000);
                chro_hi (erm);
        } else {
                form (erm, "%u", a);
                chro_lo (erm);
        }
}

void chro_xx (word hi, word a) {

        char erm [6];

        form (erm, "%x", a);

        if (hi) {
                chro_hi (erm);
        } else {
                chro_lo (erm);
        }
}
#else
void ezlcd_init () { emul (0, "(%lu) Init", seconds()); }
void ezlcd_on () { emul (0, "(%lu) ON", seconds()); }
void ezlcd_off () { emul (0, "(%lu) OFF", seconds()); }

void chro_xx (word hi, word a) {
	emul (0, "(%lu) %s: %x", seconds(), hi ? "hi" : "lo", a);
}

void chro_nn (word hi, word a) {
	if (hi)
		emul (0, "(%lu) hi: %d", seconds(), a % 10000);
	else
		emul (0, "(%lu) lo: %d", seconds(), a);
}

void chro_hi (const char *txt) { emul (0, "(%lu) hi: %s", seconds(), txt); }
void chro_lo (const char *txt) { emul (0, "(%lu) lo: %s", seconds(), txt); }

void cma3000_on (byte m, byte n, byte o) { 
	emul (0, "(%lu) cma %d, %d, %d", seconds(), m, n, o);
}

void cma3000_off () { emul (0, "(%lu) cma off", seconds()); }

void buzzer_init () { emul (0, "(%lu) buzzer_init", seconds()); }
void buzzer_on () { emul (0, "(%lu) buzzer_on", seconds()); }
void buzzer_off () { emul (0, "(%lu) buzzer_off", seconds()); }

#endif
