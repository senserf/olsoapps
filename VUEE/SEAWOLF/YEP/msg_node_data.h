#ifndef __msg_node_data_h__
#define	__msg_node_data_h__

msgBeacType	myBeac;
msgActType	myAct;

#ifdef	__SMURPH__

int _da (tr_offset) (headerType*);
Boolean _da (msg_isBind) (msg_t m);
Boolean _da (msg_isTrace) (msg_t m);
Boolean _da (msg_isMaster) (msg_t m);
Boolean _da (msg_isNew) (msg_t m);
Boolean _da (msg_isClear) (byte o);
void _da (set_master_chg) (void);

#endif

#endif
