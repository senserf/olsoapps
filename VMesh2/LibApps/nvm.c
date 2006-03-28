#include "nvm.h"
#if EEPROM_DRIVER
#include "eeprom.h"
#endif
#include "app.h"
#include "codes.h"
#include "msg_gene.h"

/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2005.                   */
/* All rights reserved.                                                 */
/* ==================================================================== */

#define NVM_NILLW	0xFFFFFFFF
lword esns [2] = {0, 0};
word  svec [SVEC_SIZE];
word esn_count = 0;

void nvm_read (word pos, word * d, word wlen) {
	if (wlen == 0 || pos + wlen >=
#if EEPROM_DRIVER
		EE_SIZE >> 1
#else
		IFLASH_SIZE
#endif
	) {
		dbg_2 (0xE000 | (byte)wlen);
		return;
	}
#if EEPROM_DRIVER
	ee_read (pos << 1, (byte *)d, wlen << 1);
#else
	while (wlen--)
		*d++ = IFLASH [pos++];
#endif
}

void nvm_write (word pos, const word * s, word wlen) {
	if (wlen == 0 || pos + wlen >=
#if EEPROM_DRIVER
		EE_SIZE >> 1
#else
		IFLASH_SIZE
#endif
	) {
		dbg_2 (0xE100 | (byte)wlen);
		return;
	}

#if EEPROM_DRIVER
	ee_write (pos << 1, (const byte *)s, wlen << 1);
#else
	while (wlen--)
		if_write (pos++, *s++);
#endif
}

void nvm_erase() {
#if EEPROM_DRIVER
	ee_erase();
#else
	if_erase();
#endif
}

extern brCtrlType br_ctrl;

static void read_esnt (word pos, lword * d, word n) {
	if (pos + n > ESN_SIZE) {
		memset (d, 0xff, n << 2);
		n = ESN_SIZE - pos;
	}
	nvm_read (NVM_PAGE_SIZE * ESN_OSET + (pos << 1), (word *)d, n << 1);
}

static void write_esnt (word pos, lword * s, word n) {
	if (pos + n > ESN_SIZE)
		 n = ESN_SIZE - pos;
	nvm_write (NVM_PAGE_SIZE * ESN_OSET + (pos << 1), (word *)s, n << 1);
}

void clr_esn () {
	lword buf [ESN_BSIZE];
	word bp, i, rr;
	bool write_back;

	read_esnt (0, buf, ESN_BSIZE);
	bp = i = rr = 0;
	write_back = NO;
	while (i < ESN_SIZE && rr < esn_count) {
		if (i >= bp + ESN_BSIZE) {
			if (write_back) {
				write_esnt (bp, buf, ESN_BSIZE);
				write_back = NO;
			}
			read_esnt (bp = i, buf, ESN_BSIZE);
		}
		if (buf[i - bp] != NVM_NILLW) {
			buf[i - bp] = NVM_NILLW;
			write_back = YES;
			esn_count--;
		}
		i++;
	}
	if (write_back)
		write_esnt (bp, buf, ESN_BSIZE);
	memset (svec, 0, SVEC_SIZE << 1);
}

word count_esn () {
	lword buf [ESN_BSIZE];
	word bp, i, cnt;

	read_esnt (0, buf, ESN_BSIZE);
	bp = i = cnt = 0;
	while (i < ESN_SIZE) {
		if (i >= bp + ESN_BSIZE)
			read_esnt (bp = i, buf, ESN_BSIZE);
		if (buf[i - bp] != NVM_NILLW)
			cnt++;
		i++;
	}
	return cnt;
}

int lookup_esn (lword * e) {
	lword buf [ESN_BSIZE];
	word bp, i, rr;

	read_esnt (0, buf, ESN_BSIZE);
	bp = i = rr = 0;
	while (i < ESN_SIZE && rr < esn_count) {
		if (i >= bp + ESN_BSIZE)
			read_esnt (bp = i, buf, ESN_BSIZE);
		if (buf[i - bp] == *e)
			return i;
		if (buf[i - bp] != NVM_NILLW)
			rr++;
		i++;
	}
	return -RC_EVAL;
}
 
int lookup_esn_st (lword * e) {
	int i = lookup_esn (e);
	return i < 0 ? i : (svec[i >> 4] >> (i & 15)) & 1;
}

#if 0

int set_esn_st (lword * e, bool what) {
	int i = lookup_esn (e);
	if (i >= 0) {
		if (what)
			svec[i >> 4] |= 1 << (i & 15);
		else
			svec[i >> 4] &= ~(1 << (i & 15));
	}
	return i;
}
#endif

int get_next (lword * e, word st) {
	lword lw;
	lword buf [ESN_BSIZE];
	word bp, i, rr, j;
	int  rb = -RC_EVAL;
	bool b;

	read_esnt (0, buf, ESN_BSIZE);
	bp = i = rr = 0;
	lw = NVM_NILLW;
	while (i < ESN_SIZE && rr < esn_count) {
		if (i >= bp + ESN_BSIZE)
			read_esnt (bp = i, buf, ESN_BSIZE);
		j = i - bp;
		if (buf[j] != NVM_NILLW) {
			rr++;
			if (buf[j] > *e && buf[j] < lw) {
				b = (svec[i >> 4] >> (i & 15)) & 1;
				if (st > 1 || st == b) {
					lw = buf[j];
					rb = b;
				}
			}
		}
		i++;
	}
	*e = lw;
	return rb;
}

bool load_esns (char * msg) {
	lword buf [ESN_BSIZE];
	lword lpack [ESN_PACK];
	lword tmp;
	word bp, i, rr, j, cnt, lptr;
	
	read_esnt (0, buf, ESN_BSIZE);
	bp = i = rr = cnt = lptr = 0;
	memset (lpack, 0xff, sizeof(lpack));
	while (i < ESN_SIZE && rr < esn_count) {
		if (i >= bp + ESN_BSIZE)
			read_esnt (bp = i, buf, ESN_BSIZE);
		j = i - bp;
		if (buf[j] != NVM_NILLW) {
			rr++;
			if (buf[j] > esns[0]) {
				if ((br_ctrl.rep_freq & 1) != 
					((svec[i >> 4] >> (i & 15)) & 1)) {
					i++;
					continue;
				}
				if (lptr < ESN_PACK) {
					lpack[lptr++] = buf[j];
					cnt++;
				} else {
					if (lpack [ESN_PACK -1] < buf[j]) {
						i++;
						continue;
					}
					lpack [ESN_PACK -1] = buf[j];
				}
				if (lptr < 2) {
					i++;
					continue;
				}
				j = lptr -1;
				while (j > 0 && lpack[j] < lpack[j-1]) {
					tmp = lpack[j];
					lpack[j] = lpack[j-1];
					lpack[j-1] = tmp;
					j--;
				}
			}
		}
		i++;
	}
	in_st(msg, count) = cnt;
	if (cnt == 0) {
		esns[1] = NVM_NILLW;
		return NO;
	}
	memcpy (msg + sizeof(msgStType), (char *)lpack, cnt << 2);
	esns[1] = lpack[cnt-1];
	return YES;
}

void set_svec (int i, bool what) {
	if (i >= 0) {
		if (what)
			svec[i >> 4] |= 1 << (i & 15);
		else
			svec[i >> 4] &= ~(1 << (i & 15));
	}
}

int add_esn (lword * e, int * pos) {
	lword buf [ESN_BSIZE];
	word bp, i, rr;

	*pos = -1;
	read_esnt (0, buf, ESN_BSIZE);
	bp = i = rr = 0;
	while (i < ESN_SIZE && rr < esn_count) {
		if (i >= bp + ESN_BSIZE)
			read_esnt (bp = i, buf, ESN_BSIZE);
		if (buf[i - bp] == *e) {
			*pos = i;
			return RC_ERES;
		}
		if ( buf[i - bp] == NVM_NILLW) {
			if (*pos == -1)
				*pos = i;
		} else
			rr++;
		i++;
	}
	if (*pos == -1) {
		if (i >= ESN_SIZE)
			return RC_ELEN;
		*pos = i;
	}
	write_esnt (*pos, e, 1);
	svec[*pos >> 4] |= 1 << (*pos & 15);
	esn_count++;
	return RC_OK;
}

int era_esn (lword * e) {
	lword buf [ESN_BSIZE]; 
	word bp, i, rr; 

	read_esnt (0, buf, ESN_BSIZE);
	bp = i = rr = 0; 
	while (i < ESN_SIZE && rr < esn_count) {
		if (i >= bp + ESN_BSIZE)
			 read_esnt (bp = i, buf, ESN_BSIZE);
		if (buf[i - bp] == *e) {
			svec[i >> 4] &= ~(1 << (i & 15));
			buf[0] = NVM_NILLW; // need an address
			write_esnt (i, buf, 1);
			esn_count--;
			return i;
		}
		i++;
	}
	return -RC_EVAL;
}

word s_count () {
	word w, c = 0, j;
	int i = SVEC_SIZE;
	while (--i >= 0) {
		w = svec[i];
		for (j = 0; j < 16; j++)
			if ((w >> j) &1)
				c++;
	}
	return c;
}

void app_reset (word lev) {
	word a[4];
	if (lev & 8) {
		memset (a, 0xFF, 8);
		nvm_write (NVM_NID, a, 4);
		clr_esn();
		reset();
	}
	if (lev & 2)
		clr_esn();
	else if (lev & 1)
		memset (svec, 0, SVEC_SIZE << 1);
	if (lev & 4)
		 reset();
}
#undef NVM_NILLW

