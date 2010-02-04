/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2009                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#ifndef __SMURPH__
// dupa vuee barfs, must be included twice 
#include "app_ert.h"
#endif

#include "tarp.h"

#ifdef __dcx_def__

#ifndef	__app_ert_data_defined__
#define	__app_ert_data_defined__

__CONST lword	ESN		__sinit (0xBACA0001);
lword		cyc_sp;
lword		cyc_left	__sinit (0);
lword		io_creg;
lword		io_pload	__sinit (0xFFFFFFFF);
//nid_t		net_id, local_host, master_host;
word		app_flags, freqs, connect, l_rssi;
byte		* cmd_line	__sinit (NULL);
byte		* dat_ptr	__sinit (NULL);
byte		dat_seq		__sinit (0);
// spare byte
cmdCtrlType	cmd_ctrl;
// dupa cmdCtrlType	cmd_ctrl	__sinit ({0, 0x00, 0x00, 0x00, 0x00});
brCtrlType	br_ctrl;
cycCtrlType	cyc_ctrl;
sint		shared_left;

sint 		rcv_packet_size __sinit (0);
char 		* rcv_buf_ptr	__sinit (NULL);
word		rcv_rssi	__sinit (0);

char		* outs_ptr;
sint		outs_len;

lword		iob_htime;

sint		ior_left;

#if UART_DRIVER || UART_TCV

#if UART_TCV
sint 		OSFD;
#endif

#define UI_INLEN 	UART_INPUT_BUFFER_LENGTH
byte		uart_ibuf [UI_INLEN], uart_oset, uart_len;
#endif

int _da (tr_offset) (headerType*);
Boolean _da (msg_isBind) (msg_t m);
Boolean _da (msg_isTrace) (msg_t m);
Boolean _da (msg_isMaster) (msg_t m);
Boolean _da (msg_isNew) (msg_t m);
Boolean _da (msg_isClear) (byte o);
void _da (set_master_chg) (void);

#endif

#endif

// ============================================================================

#ifdef __dcx_dcl__

#ifdef __SMURPH__

#define ESN             _daprx (ESN)
#define cyc_sp          _daprx (cyc_sp)
#define cyc_left        _daprx (cyc_left)
#define io_creg         _daprx (io_creg)
#define io_pload        _daprx (io_pload)
//#define net_id		_daprx (net_id)
//#define local_host	_daprx (local_host)
//#define master_host	_daprx (master_host)
#define app_flags       _daprx (app_flags)
#define freqs           _daprx (freqs)
#define connect         _daprx (connect)
#define l_rssi          _daprx (l_rssi)
#define cmd_line        _daprx (cmd_line)
#define dat_ptr         _daprx (dat_ptr)
#define dat_seq         _daprx (dat_seq)
#define cmd_ctrl        _daprx (cmd_ctrl)
#define br_ctrl		_daprx (br_ctrl)
#define cyc_ctrl        _daprx (cyc_ctrl)
#define shared_left     _daprx (shared_left)
#define rcv_packet_size _daprx (rcv_packet_size)
#define rcv_buf_ptr     _daprx (rcv_buf_ptr)
#define rcv_rssi        _daprx (rcv_rssi)
#define outs_ptr        _daprx (outs_ptr)
#define outs_len	_daprx (outs_len)
#define iob_htime       _daprx (iob_htime)
#define ior_left        _daprx (ior_left)

#if UART_DRIVER || UART_TCV

#if UART_TCV
#define OSFD		_daprx (OSFD)
#endif

#define uart_ibuf	_daprx (uart_ibuf)
#define uart_oset	_daprx (uart_oset)
#define uart_len	_daprx (uart_len)
#endif

#else
// PicOS
//+++ app_tarp_ert.c lib_app_ert.c msg_io_ert.c oss_io_ert.c app_ser_ert.c
//+++ nvm_ert.c

extern const lword	ESN;
extern lword           cyc_sp;
extern lword           cyc_left;
extern lword           io_creg;
extern lword           io_pload;
//extern nid_t	       net_id, local_host, master_host;
extern word            app_flags, freqs, connect, l_rssi;
extern byte            * cmd_line;
extern byte            * dat_ptr;
extern byte            dat_seq;
extern cmdCtrlType     cmd_ctrl;
extern brCtrlType      br_ctrl;
extern cycCtrlType     cyc_ctrl;
extern sint            shared_left;
extern sint            rcv_packet_size;
extern char            * rcv_buf_ptr;
extern word            rcv_rssi;
extern char            * outs_ptr;
extern sint            outs_len;
extern lword           iob_htime;
extern sint            ior_left;

#if UART_DRIVER || UART_TCV

#if UART_TCV
extern sint		OSFD;
#endif

extern byte            uart_ibuf[], uart_oset, uart_len;
#endif

// PicOS
#endif

char * get_mem (word state, int len);

int app_ser_out (word st, char * m, bool cpy);

void msg_cmd_in (word state, char * buf);
void msg_master_in (char * buf);
void msg_trace_in (word state, char * buf);
void msg_traceAck_in (word state, char * buf);
void msg_bind_in (char * buf);
void msg_bindReq_in (char * buf);
void msg_new_in (char * buf);
void msg_alrm_in (char * buf);
void msg_br_in (char * buf);
void msg_stAck_in (char * buf);
void msg_io_in (char * buf);
void msg_ioAck_in (char * buf);
void msg_dat_in (char * buf);
void msg_datAck_in (char * buf);
void msg_nh_in (char * buf, word rssi);
void msg_nhAck_in (char * buf);

void msg_cmd_out (word state, char** buf_out);
void msg_master_out (word state, char** buf_out);
void msg_trace_out (word state, char** buf_out);
word msg_traceAck_out (word state, char *buf, char** out_buf);
void msg_bind_out (word state, char** buf_out);
bool msg_bindReq_out (char * buf, char** buf_out);
bool msg_new_out ();
bool msg_alrm_out (char * buf);
bool msg_br_out();
bool msg_stAck_out (char * buf);
bool msg_io_out ();
bool msg_ioAck_out (char * buf);
word msg_dat_out ();
bool msg_datAck_out (char * buf);
bool msg_nh_out ();
bool msg_nhAck_out (char * buf, char** buf_out, word rssi);

void send_msg (char * buf, int size);

void oss_ret_out (word state);

void oss_trace_in (word state);
void oss_traceAck_out (word state, char * buf);
void oss_bindReq_out (char * buf);
void oss_alrm_out (char * buf);
void oss_br_out (char * buf, bool acked);
void oss_io_out (char * buf, bool acked);
void oss_dat_out (char * buf, bool acked);
void oss_datack_out (char * buf);
void oss_nhAck_out (char * buf, word rssi);

void oss_master_in (word state);
void oss_set_in ();
void oss_get_in (word state);
void oss_bind_in (word state);
void oss_sack_in ();
void oss_io_in ();
void oss_ioack_in ();
void oss_dat_in ();
void oss_datack_in ();
void oss_reset_in ();
void oss_locale_in ();
void app_leds (const word act);

void nvm_read (word pos, address d, word wlen);
void nvm_write (word pos, const word * s, word wlen);
void nvm_erase();
void nvm_io_backup();
void app_reset (word lev);

Boolean _da (msg_isMaster) (msg_t m);

#endif

// ============================================================================

#ifdef __dcx_ini__

	ESN		= (lword) preinit ("HID");
        cyc_left	= 0;
        io_pload	= 0xFFFFFFFF;
        cmd_line	= NULL;
        dat_ptr		= NULL;
        dat_seq		= 0;
	memset (&cmd_ctrl, 0, sizeof(cmdCtrlType));
        rcv_packet_size = 0;
        rcv_buf_ptr	= NULL;
        rcv_rssi	= 0;

#endif
