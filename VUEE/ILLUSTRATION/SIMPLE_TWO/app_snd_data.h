// ============================================================================

#ifdef __dcx_def__

#ifndef	__app_snd_data_defined__
#define	__app_snd_data_defined__

int	sfd __sinit (-1);
address	spkt;
char	*ibuf;

#endif

#endif

// ============================================================================

#ifdef __dcx_dcl__

#ifdef __SMURPH__

#define	sfd	_daprx (sfd)
#define	spkt	_daprx (spkt)
#define	ibuf	_daprx (ibuf)

#else

extern	int sfd;
extern	address spkt;
extern	char *ibuf;

#endif

#endif

// ============================================================================

#ifdef __dcx_ini__

	sfd = -1;

#endif
