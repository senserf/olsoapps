/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/

// for now, a direct copy from CHRONOS/RFTEST

#include "sysio.h" // missing in ez430_lcd.h
#include "ez430_lcd.h"
#include "form.h"

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
                form (erm, "%u", a % 9999);
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

