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
// like the command being acknowledged. The role of SER (serial number) is to 
// match ACKs to commands.
//
// A simple ACK response consists of a single word - the command status: zero
// if OK, error number if something wrong (WNONE is reserved). A longer
// response means a packet count report which looks like this:
//
//	SENT DRVT DRVC DRVL DRVS NNOD [Node Count]*N
//	SENT RERR-6 NNOD [Node Count]*NNOD
//
// where:
//
//	SENT	is the total number of measurement packets sent by the node
//
//	RERR	is the 6-word PHYSOPT_ERROR table of the sending node
//
//	NNOD	is the number of nodes <= 8 from which the node received
//		measurement packets; this is followed by that many pairs
//		<Node, Count> representing counts of packets received from
//		the individual nodes
//

#define	POFF_SENT	(POFF_SER+1)
#define	POFF_RERR	(POFF_SER+2)
#define	POFF_NNOD	(POFF_SER+8)
#define	POFF_NTAB	(POFF_SER+9)

// POFF_NTAB == 12, i.e., 12 words so far, i.e., 24 bytes, which leaves us 
// 60 - 24 - 2 [CHS] = 34 bytes for NTAB, limiting MAX_NODES to 8 (with two
// spare bytes)
//
// Note that driver reports refer to all packets that the node tried to
// receive, not only measurement packets, and possibly some stray packets
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

#define	MIN_ANY_PACKET_LENGTH	((POFF_FLG + 2)*2)
#define	MIN_ACK_LENGTH		12
#define	MIN_MES_PACKET_LENGTH	((POFF_FLG + 2 + NUMBER_OF_SENSORS) * 2)

// ============================================================================

#define	PATABLE_MAX_TRIES	16
#define	PATABLE_REPLY_DELAY	1024
#define	MIN_SEND_INTERVAL	8
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

// ============================================================================

extern const lword host_id;

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

fsm thread_rfcmd, thread_rreporter, thread_ureporter (word), thread_listener,
	thread_sender, thread_patable, thread_chguard, thread_pcmnder;

#endif
