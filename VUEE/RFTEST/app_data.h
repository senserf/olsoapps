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

fsm thread_rfcmd;
fsm thread_rreporter;
fsm thread_ureporter;
fsm thread_listener;
fsm thread_sender;

#endif
