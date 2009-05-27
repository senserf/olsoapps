#ifndef	__picos_threadhdrs_h__
#define	__picos_threadhdrs_h__

#ifdef	__SMURPH__

process THREADNAME (root) (Node) {

	char *ibuf;
	int i;
	int SFD;

	states {
		RS_INIT,
		RS_RCMD,
		RS_ECHO
	};

	perform;
};

#else

static	char *ibuf;
static	int i;
static  int SFD;

#define	RS_INIT		0
#define	RS_RCMD		1
#define	RS_ECHO		2

#endif
#endif
