/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2009.                   */
/* All rights reserved.                                                 */
/* ==================================================================== */

static  void s2d (mdate_t * in) {
	lint l1;
	word w1, w2;

	if (in->secs < 0) {
		l1 = -in->secs;
		in->dat.f = 1;
	} else {
		l1 = in->secs;
		in->dat.f = 0;
	}

	w1 = (word)(l1 / SIY);
	l1 -= SIY * w1 + SID * (w1 >> 2); // leap days in prev. years

	if (l1 < 0) {
		l1 += SIY;
		if (w1-- % 4 == 0)
			l1 += SID;
	}

	w2 = (word)(l1 / SID); // days left base 0

	if (w2 < 31) {
		in->dat.mm = 1;
		l1 -= SID * w2++;
		goto Fin;
	}

	if (w2 < 59 || ((w1 +1) % 4 == 0 && w2 < 60)) { // stupid comp warnings
		in->dat.mm = 2;
		l1 -= SID * w2;
		w2 -= 30;
		goto Fin;
	}

	if ((w1 +1) % 4 != 0) {
		w2++; // equal non-leap years
		l1 += SID;
	}

	if (w2 < 91) {
		in->dat.mm = 3;
		l1 -= SID * w2; 
		w2 -= 59;
		goto Fin;
	}

	if (w2 < 121) {
		in->dat.mm = 4;
		l1 -= SID * w2;
		w2 -= 90;
		goto Fin;
	}

	if (w2 < 152) {
		in->dat.mm = 5;
		l1 -= SID * w2;
		w2 -= 120;
		goto Fin;
	}

	if (w2 < 182) {
		in->dat.mm = 6;
		l1 -= SID * w2;
		w2 -= 151;
		goto Fin;
	}

	if (w2 < 213) {
		in->dat.mm = 7;
		l1 -= SID * w2;
		w2 -= 181;
		goto Fin;
	}

	if (w2 < 244) {
		in->dat.mm = 8;
		l1 -= SID * w2;
		w2 -= 212;
		goto Fin;
	}

	if (w2 < 274) {
		in->dat.mm = 9;
		l1 -= SID * w2;
		w2 -= 243;
		goto Fin;
	}

	if (w2 < 305) {
		in->dat.mm = 10;
		l1 -= SID * w2;
		w2 -= 273;
		goto Fin;
	}

	 if (w2 < 335) {
		 in->dat.mm = 11;
		 l1 -= SID * w2;
		 w2 -= 304;
		 goto Fin;
	 }

	 if (w2 < 366) {
		 in->dat.mm = 12;
		 l1 -= SID * w2;
		 w2 -= 334;
		 goto Fin;
	 }

	// error
	in->dat.f = 0;
	in->dat.h = 0;
	in->dat.m = 0;
	in->dat.s = 0;
	in->dat.yy = 0;
	in->dat.mm = 1;
	in->dat.yy = 1;
	return;


Fin:
	in->dat.h = (word)(l1 / 3600);
	l1 %= 3600;
	in->dat.m = (word)(l1 / 60);
	in->dat.s = l1 % 60;
	in->dat.yy = w1;
	in->dat.dd = w2;
}

static  void d2s (mdate_t * in) {
  lint l1 = 365L * in->dat.yy - 365 + (in->dat.yy >> 2) +
	  in->dat.dd + 30 * in->dat.mm - 30; // days

  if (in->dat.yy % 4 == 0 && in->dat.mm < 3)
	  l1--;

  switch (in->dat.mm) {
	  case 2:
	  case 6:
	  case 7:
		  l1++;
		  break;
	  case 9:
	  case 10:
		  l1 += 3;
		  break;
	  case 11:
	  case 12:
		  l1 += 4;
		  break;
	  case 3:
		  l1--;
		  break;
	  case 8:
		  l1 += 2;
  }

  // - SID below makes 9-1-1 day 0
  l1 = l1 * SID + 3600L * in->dat.h + 60 * in->dat.m + in->dat.s - SID;

  in->secs = in->dat.f ? -l1 : l1;
}

