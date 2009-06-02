#ifndef	__picos_threadhdrs_h__
#define	__picos_threadhdrs_h__

#ifdef	__SMURPH__

process THREADNAME (outlines) (Node) {

	const char *data;
	
	states { OL_INIT, OL_NEXT };

	void setup (const char *d) { data = (const char*) d; };

	void savedata (void *d) { data = (const char*) d; };

	perform;
};

process THREADNAME (eetest) (Node) {

	states {
			EP_INIT,
			EP_RCMD,
			EP_RCMD1,
			EP_SWO,
			EP_SWO1,
			EP_SLW,
			EP_SLW1,
			EP_SST,
			EP_SST1,
			EP_RWO,
			EP_RWO1,
			EP_RLW,
			EP_RLW1,
			EP_RST,
			EP_RST1,
			EP_WRI,
			EP_WRI1,
			EP_REA,
			EP_ERA,
			EP_SYN,
			EP_DIA,
			EP_FLR,
			EP_FLW,
			EP_FLE,
			EP_ETS,
			EP_ETS_O,
			EP_ETS_E,
			EP_ETS_F,
			EP_ETS_G,
			EP_ETS_M,
			EP_ETS_H,
			EP_ETS_I,
			EP_ETS_N,
			EP_ETS_J,
			EP_ETS_K,
			EP_ETS_L
	};

	perform;
};

process THREADNAME (root) (Node) {

	states {
			RS_INIT,
			RS_RESTART,
			RS_RCMD,
			RS_RCMD1
	};

	perform;
};

#else

// ============================================================================

#define	OL_INIT		0
#define	OL_NEXT		1

// ============================================================================

#define EP_INIT		0
#define EP_RCMD		4
#define EP_RCMD1	5
#define EP_SWO		6
#define EP_SWO1		7
#define EP_SLW		8
#define EP_SLW1		9
#define EP_SST		10
#define EP_SST1		11
#define EP_RWO		12
#define EP_RWO1		13
#define EP_RLW		14
#define EP_RLW1		15
#define EP_RST		16
#define EP_RST1		17
#define EP_WRI		18
#define EP_WRI1		19
#define EP_REA		20
#define EP_ERA		21
#define EP_SYN		22
#define EP_DIA		23
#define EP_FLR		24
#define EP_FLW		25
#define EP_FLE		26
#define EP_ETS		27
#define EP_ETS_O	28
#define EP_ETS_E	29
#define EP_ETS_F	30
#define EP_ETS_G	31
#define EP_ETS_M	32
#define EP_ETS_H	33
#define EP_ETS_I	34
#define EP_ETS_N	35
#define EP_ETS_J	36
#define EP_ETS_K	37
#define EP_ETS_L	38

// ============================================================================

#define	RS_INIT		0
#define	RS_RESTART	1
#define	RS_RCMD		2
#define	RS_RCMD		3

// ============================================================================

int SFD, b;
char * ibuf, * obuf;

lword adr, val, u, s, pat;
word w, err, len, bs, nt, sl, ss;
word dcnt;
byte str [72], * blk;

#endif
#endif
