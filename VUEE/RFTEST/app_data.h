// ============================================================================

#ifndef	MAX_NODES

#define	MAX_NODES		8

// ============================================================================
//
// Command packet format (words):
//
//	NID RCV SND SER ... cmd ... CHK

#define	POFF_NID	0	// Offsets in words
#define	POFF_RCV	1	// Recipient (zero for a measurement packet)
#define	POFF_SND	2	// Sender
#define	POFF_SER	3	// Serial number (for a command)
#define	POFF_FLG	3	// Flag field (measurement packet)
#define	POFF_SEN	4	// First sensor location (measurement packet)
#define	POFF_CMD	4	// Command code (command packet)

// Maximum command length in a command packet: ==
// MAX - header - CHK - string sentinel
#define	MAX_RF_CMD_LENGTH	(MAX_PACKET_LENGTH - ((POFF_CMD + 1) * 2) - 1)

//
// Command acknowledgments look like commands:
//
//	NID RCV SND SER [... response ...] CHK
//
// except that RCV <-> SND are interchanged, i.e., the first four words look
// the command being acknowledged. The role of SER (serial number) is to 
// match ACKs to commands.
//
// A simple ACK response consists of a single word - the command status: zero
// if OK, error number if something wrong (WNONE is reserved). A longer
// response means a packet count report which looks like this:
//
//	SENT DRVT DRVC DRVL DRVS NNOD [Node Count]*N
//
// where:
//
//	SENT	is the total number of measurement packets sent by the node
//
//	DRVT	is the total number of reception attempts reported by the
//		driver
//
//	DRVC	is the total number of checksum errors reported by the driver
//
//	DRVL	is the total number of length errors reported by the driver
//
//	DRVS	is the total number of wrong NETIDs reported by the driver
//
//	NNOD	is the number of nodes <= 8 from which the node received
//		measurement packets; this is followed by that many pairs
//		<Node, Count> representing counts of packets received from
//		the individual nodes
//

#define	POFF_SENT	(POFF_SER+1)
#define	POFF_DRVT	(POFF_SER+2)
#define	POFF_DRVC	(POFF_SER+3)
#define	POFF_DRVL	(POFF_SER+4)
#define	POFF_DRVS	(POFF_SER+5)
#define	POFF_NNOD	(POFF_SER+6)
#define	POFF_NTAB	(POFF_SER+7)

// POFF_NTAB == 10, i.e., 10 words so far, i.e., 20 bytes, which leaves us 
// 60 - 20 - 2 [CHS] = 38 bytes for NTAB, limiting MAX_NODES to 9 (presently
// set at 8 for a round limit).
//
// Note that driver reports refer to all packets that the node tried to
// receive, not only measurement packates, and possibly some stray packets
// from unrelated devices.
//
// Measurement packets (the ones to be counted) look like this:
//
//	NID 0 SND FLG SEN1 ... SENN ... random contents ... CHK
//
// The total length of a packet to send is a random number between
// g_pkt_minpl and g_pkt_maxpl, inclusively, with the actual minimum never
// below MIN_MES_PACKET_LENGTH, which accounts for all the declared sensors.
// FLG is xmit power level with 0x8000 or'red in, if the node operates in PD
// mode.

#define	MIN_ANY_PACKET_LENGTH	((POFF_FLG + 2) * 2)
#define	MIN_ACK_LENGTH		12
#define	MIN_MES_PACKET_LENGTH	((POFF_FLG + 2 + NUMBER_OF_SENSORS) * 2)

// ============================================================================

#define	MIN_SEND_INTERVAL	256
#define	UART_LINE_LENGTH	82
#define	RF_COMMAND_SPACING	1024
#define	RF_COMMAND_RETRIES	5
#define	NETWORK_ID		0xF610
#define	HOST_ID			((word) host_id)
#define	HOST_FLAGS		((word) (host_id >> 16))

typedef struct {

	word	Node;
	word	Count;

} noded_t;

#endif

// ============================================================================

#ifdef __dcx_def__

#ifndef	__app_data_defined__
#define	__app_data_defined__

int	g_fd_rf __sinit (-1), g_fd_uart __sinit (-1), g_snd_opl,
	g_pkt_minpl __sinit (MIN_MES_PACKET_LENGTH),
	g_pkt_maxpl __sinit (MAX_PACKET_LENGTH);

word	g_pkt_mindel __sinit (1024), g_pkt_maxdel __sinit (1024), g_rep_cnt,
	g_snd_count __sinit (0), g_snd_rnode __sinit (0), g_snd_rcode,
	g_snd_sernum __sinit (1), g_snd_rtries, g_err_stat, g_flags __sinit (0),
	g_sen_cnt;

char	*g_snd_rcmd __sinit (NULL);

byte	*g_reg_suppl __sinit (NULL);

address	g_rcv_ackrp __sinit (NULL), g_snd_pkt;

noded_t	g_rep_nodes [MAX_NODES];

#ifdef __SMURPH__
lword 	host_id;
#endif

#endif

#endif

// ============================================================================

#ifdef __dcx_dcl__

#ifdef __SMURPH__

#define	g_fd_rf		_daprx (g_fd_rf)
#define	g_fd_uart	_daprx (g_fd_uart)
#define	host_id		_daprx (host_id)
#define	g_snd_opl	_daprx (g_snd_opl)
#define	g_rcv_ackrp	_daprx (g_rcv_ackrp)
#define	g_pkt_minpl	_daprx (g_pkt_minpl)
#define	g_pkt_maxpl	_daprx (g_pkt_maxpl)
#define	g_pkt_mindel	_daprx (g_pkt_mindel)
#define	g_pkt_maxdel	_daprx (g_pkt_maxdel)
#define	g_rep_cnt	_daprx (g_rep_cnt)
#define	g_snd_rnode	_daprx (g_snd_rnode)
#define	g_snd_sernum	_daprx (g_snd_sernum)
#define	g_snd_rcmd	_daprx (g_snd_rcmd)
#define	g_snd_rtries	_daprx (g_snd_rtries)
#define	g_snd_rcode	_daprx (g_snd_rcode)
#define	g_snd_count	_daprx (g_snd_count)
#define	g_snd_pkt	_daprx (g_snd_pkt)
#define	g_sen_cnt	_daprx (g_sen_cnt)
#define	g_err_stat	_daprx (g_err_stat)
#define	g_flags		_daprx (g_flags)
#define	host_id		_daprx (host_id)
#define	g_rep_nodes	_daprx (g_rep_nodes)
#define	g_reg_suppl	_daprx (g_reg_suppl)

#else

extern	int 	g_fd_rf, g_fd_uart, g_snd_opl, g_pkt_minpl, g_pkt_maxpl;
extern	word	g_pkt_mindel, g_pkt_maxdel, g_snd_count, g_sen_cnt;
extern	lword	host_id;
extern	address	g_rcv_ackrp, g_snd_pkt;
extern	word	g_rep_cnt, g_snd_rnode, g_snd_sernum, g_snd_rtries, g_err_stat,
		g_snd_rcode, g_flags;
extern	char	*g_snd_rcmd;
extern	noded_t	g_rep_nodes [];
extern	byte	*g_reg_suppl;

#endif

word report_size ();
void reset_count ();
void uart_outf (word, const char*, ...);
void uart_out (word, const char*);
void confirm (word, word, word);
void handle_ack (address, word);
word do_command (const char*, word, word);
word gen_send_params ();
void update_count (word);
void reset_count ();
word report_size ();
void view_packet (address, word);

#endif

// ============================================================================

#ifdef __dcx_ini__

g_fd_rf = g_fd_uart = -1;
g_pkt_minpl = MIN_MES_PACKET_LENGTH;
g_pkt_maxpl = MAX_PACKET_LENGTH;
g_pkt_mindel = g_pkt_maxdel = 1024;
g_snd_sernum = 1;
g_snd_rnode = g_snd_count = g_flags = 0;
g_snd_rcmd = NULL;
g_rcv_ackrp = NULL;
g_reg_suppl = NULL;
memset (g_rep_nodes, 0, sizeof (g_rep_nodes));
host_id = (lword) preinit ("HID");

#endif
