#include "nvm.h"
#if EEPROM_DRIVER
#include "eeprom.h"
#endif
#include "app.h"
#include "app_tarp_if.h"
#include "codes.h"
#include "msg_vmesh.h"
#include "lib_app_if.h"
#include "pinopts.h"

/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2006.                   */
/* All rights reserved.                                                 */
/* ==================================================================== */

#define NVM_NILLW	0xFFFFFFFF
lword esns [2] = {0, 0};
word  svec [SVEC_SIZE];
word esn_count = 0;


#if EEPROM_DRIVER
#define nvm_erase(a)	ee_erase()
#else
#define nvm_erase(a)	do { \
				if_erase(a); \
				dbg_f(0xF000 | (a)); \
			} while (0)
#endif

void nvm_read (word pos, address d, word wlen) {
	if (wlen == 0 || pos + wlen >
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

#if !EEPROM_DRIVER
static void fpage_reset (word p) {
	lword lw;
	if (p & 0xFFC0) { // ESN page: should not be
		dbg_2 (0xE200 | p);
		nvm_erase (-1);
	} else
		nvm_erase (p);
	if_write (NVM_NID, net_id);
	if_write (NVM_LH, local_host);
	if_write (NVM_MID, master_host);
	if_write (NVM_APP, encr_data);
	memcpy (&lw, &cyc_ctrl, 2);
	if_write (NVM_CYC_CTRL, (word)lw);
	if_write (NVM_CYC_SP, (word)cyc_sp);
	if_write (NVM_CYC_SP +1, (word)(cyc_sp >> 16));
	lw = pmon_get_cmp();
	if_write (NVM_IO_CMP, (word)lw);
	if_write (NVM_IO_CMP +1, (word)(lw >> 16));
	if_write (NVM_IO_CREG, (word)io_creg);
	if_write (NVM_IO_CREG +1, (word)(io_creg >> 16));
	lw = pmon_get_cnt() << 8 | pmon_get_state();
	if_write (NVM_IO_STATE, (word)lw);
	if_write (NVM_IO_STATE +1, (word)(lw >> 16));
}
#endif

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
#endif
}

extern brCtrlType br_ctrl;

static void read_esnt (word pos, lword * d, word n) {
	if (pos + n > ESN_SIZE) {
		memset (d, 0xff, n << 2);
		n = ESN_SIZE - pos;
	}
	nvm_read (NVM_PAGE_SIZE * ESN_OSET + (pos << 1), (address)d, n << 1);
}

static void write_esnt (word pos, lword * s, word n) {
	if (pos + n > ESN_SIZE)
		 n = ESN_SIZE - pos;
	nvm_write (NVM_PAGE_SIZE * ESN_OSET + (pos << 1), (address)s, n << 1);
}

void clr_esn () {
#if EEPROM_DRIVER
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
#else
	nvm_erase (69);
#endif
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
#if EEPROM_DRIVER
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
#endif
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
	if (lev & 8) {
		nvm_erase(-1);
		reset();
	}
	if (lev & 2)
		clr_esn();
	else if (lev & 1)
		memset (svec, 0, SVEC_SIZE << 1);
	if (lev & 4)
		 reset();
}

void nvm_io_backup () {
#if EEPROM_DRIVER
	lword lw = pmon_get_cmp();
	nvm_write (NVM_IO_CMP, (address)&lw, 2);
	nvm_write (NVM_IO_CREG, (address)&io_creg, 2);
	lw = pmon_get_cnt() << 8 | pmon_get_state();
	nvm_write (NVM_IO_STATE, (address)&lw, 2);
#else
/*
	lword lw;
	word i = NVM_IO_STATE;
	while (IFLASH[i] != 0xFFFF) {
		if ((i += 2) >=  NVM_PAGE_SIZE -1) {
			fpage_reset (0);
			return;
		}
	}
	lw = pmon_get_cnt() << 8 | pmon_get_state();
	nvm_write (i, (address)&lw, 2);
*/
	lword lw = pmon_get_cnt() << 8 | pmon_get_state();
	word i = NVM_IO_STATE;
	while (if_write (i, (word)lw) == -1 ||
		if_write (i +1, (word)(lw >> 16)) == -1)
			if ((i += 2) >=  NVM_PAGE_SIZE -1) {
				fpage_reset (0);
				return;
			}
#endif
}
#undef NVM_NILLW

