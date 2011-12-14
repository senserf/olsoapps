#ifndef	__picos_threadhdrs_h__
#define	__picos_threadhdrs_h__

#ifdef	__SMURPH__

threadhdr (root, Node) {

	char ibuf [IBUFLEN];
	byte str [IBUFLEN], *blk;
	word w, err, len, bs, nt, sl, ss;
	int b;
	lword s, a, u, lw, pat;

	states {
		RS_INIT,
		RS_RCMD_M,
		RS_RCMD,
		RS_RCMD_E,
		RS_SWO,
		RS_SWO_A,
		RS_SLW,
		RS_SLW_A,
		RS_SST,
		RS_SST_A,
		RS_RWO,
		RS_RWO_A,
		RS_RLW,
		RS_RLW_A,
		RS_RST,
		RS_RST_A,
		RS_WRI,
		RS_WRI_A,
		RS_REA,
		RS_ERA,
		RS_SYN,
		RS_FLR,
		RS_FLR_A,
		RS_FLW,
		RS_FLE,
		RS_ETS,
		RS_ETS_O,
		RS_ETS_E,
		RS_ETS_F,
		RS_ETS_G,
		RS_ETS_M,
		RS_ETS_H,
		RS_ETS_I,
		RS_ETS_N,
		RS_ETS_J,
		RS_ETS_K,
		RS_ETS_L,
		RS_SIZ,
		RS_SIZ_A
	};

	perform;
};

#else

static	char ibuf [IBUFLEN];
static	byte str [IBUFLEN], *blk;
static	word w, err, len, bs, nt, sl, ss;
static	int b;
static	lword s, a, u, lw, pat;

#define	RS_INIT		0
#define	RS_RCMD_M	1
#define	RS_RCMD		2
#define	RS_RCMD_E	3
#define	RS_SWO		4
#define	RS_SWO_A	5
#define	RS_SLW		6
#define	RS_SLW_A	7
#define	RS_SST		8
#define	RS_SST_A	9
#define	RS_RWO		10
#define	RS_RWO_A	11
#define	RS_RLW		12
#define	RS_RLW_A	13
#define	RS_RST		14
#define	RS_RST_A	15
#define	RS_WRI		16
#define	RS_WRI_A	17
#define	RS_REA		18
#define	RS_ERA		19
#define	RS_SYN		20
#define	RS_FLR		21
#define	RS_FLR_A	22
#define	RS_FLW		23
#define	RS_FLE		24
#define	RS_ETS		25
#define	RS_ETS_O	26
#define	RS_ETS_E	27
#define	RS_ETS_F	28
#define	RS_ETS_G	29
#define	RS_ETS_M	30
#define	RS_ETS_H	31
#define	RS_ETS_I	32
#define	RS_ETS_N	33
#define	RS_ETS_J	34
#define	RS_ETS_K	35
#define	RS_ETS_L	36
#define	RS_SIZ		37
#define	RS_SIZ_A	38

#endif
#endif
