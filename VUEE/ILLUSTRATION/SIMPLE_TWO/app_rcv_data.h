// ============================================================================

#ifdef __dcx_def__

#ifndef	__app_rcv_data_defined__
#define	__app_rcv_data_defined__

int	sfd __sinit (-1);
word	Count;
address	rpkt;

#endif

#endif

// ============================================================================

#ifdef __dcx_dcl__

#ifdef __SMURPH__

#define	sfd	_daprx (sfd)
#define	Count	_daprx (Count)
#define	rpkt	_daprx (rpkt)

#else

extern	int sfd;
extern	word Count;
address	rpkt;

#endif

#endif

// ============================================================================

#ifdef __dcx_ini__

	sfd = -1;
	Count = 0;

#endif
