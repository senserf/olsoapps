// This is the ID to be used for the collector; needed by autoconnect, i.e.,
// the procedure of automatically locating the UART device to which the Peg
// is attached
#define	OSS_PRAXIS_ID		0x41be4010

// This message (opcode zero) is always reserved; in OSS to Peg direction, it
// is a poll to which the node has to respond with an identifying heartbeat;
// in Peg to OSS direction, the message is the heartbeat; the node can always
// safely send such a message to mainfest its presence, although normally it
// should only do it in response to a poll
#define	OPC_HEARTBEAT		0x00

// In this simple case, we only need one more message type - to be sent from
// Peg to OSS, i.e., the sample
#define	OPC_SAMPLE		0x01

// Maximum length of OSS packet
#define	OSS_PACKET_LENGTH	82

typedef struct {
	// Message header
	byte code, ref;
} oss_hdr_t;

typedef struct {
	// Sample excluding the message header
	word	peg, tag, ser;
	byte	rss [32];
} msg_sample_t;
