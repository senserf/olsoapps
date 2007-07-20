#ifndef __codes_h
#define __codes_h
/*********************************************************************
DRAFT ... IN DEVELOPMENT ... DRAFT
Routing Tags @RFM, host <-> radio RS232 i/f.

Instructions go host -> radio; 
Responses go radio -> host;
Msg ouput goes radio -> host (so far, trace only)

====================================================================
instruction:
	0x00
	i_len
	i_proxy_node
	i_target_node
	i_operation
	EOT (0x04)

i_len:
	byte (8..255)	// most likely limited to 8..61
       			// so the line is 11..64 chars

i_proxy_node:
	i_addr_qual
	node_id

i_target_node:
	i_addr_qual
	node_id

i_operation:
	op_code
	op_ref
	i_op_payload

i_addr_qual:
	byte:
		ADQ_NONE |
		ADQ_LOCAL |
		ADQ_MASTER

node_id:
	word	// little endian, i.e. node 7 is 0x07 0x00

op_code:
	byte:

		CMD_INFO |
		CMD_SET |
		CMD_MASTER |
		CMD_DISP |
		CMD_TRACE |
		CMD_BIND |
		CMD_SLEEP 
(They change from applet to applet)

op_ref:
	byte

op_len:
	byte

i_op_payload:
	byte[i_len - 8]
=====================================================================

response:
	0x00
	r_len
	r_src_node
	r_proxy_node
	r_target_node
	r_operation
	EOT (0x04)

r_len:
	byte (12..255)	// most likely 12..65 so the line is 15..68 chars

r_src_node:
	r_addr_qual
	node_id

r_proxy_node:
	r_addr_qual
	node_id

r_target_node:
	r_addr_qual
	node_id

r_operation:
	op_code
	op_ref
	op_rc
	r_op_payload

op_rc:
	byte:
		RC_OK |
		RC_NONE |
		RC_ENET |
		RC_EMAS |
		RC_ECMD |
		RC_EPAR |
		RC_EVAL |
		RC_ELEN

r_addr_qual:
	byte:
		i_addr_qual |
		ADQ_MSG |
		ADQ_OSS


r_op_payload:
	byte[r_len - 12]
========================================================================
e.g.

0x00 0x08 0x00 0x07 0x00 0x00 0x00 0x00 0x01 0xAA 0x04
entered via RS232 on node 1 will:
- send msg_cmd from 1 to 7,
- display this response on node 1:
  0x00 0x0C 0x20 0x00 0x00 0x00 0x07 0x00 0x00 0x00 0x00 0x01 0xAA 0x00 0x04,

msg_cmd processed on node 7 will:
- broadcast msg_master,
- display a similar response on 7,
(More responses should follow the broadcast, but they're blocked; even in 
 this blueprint it was seen as excessive.)

This blueprint is open to all sorts of abuse, most of which is
extremely easy to eliminate in concrete applets, or even in a family
of applets, e.g. the one described in the "Mid Size Mesh Net" RFQ.
=========================================================================

The "sign bit" in ADQ is important, as sometimes the address is replaced
 with ADQ_. So, we effectively restrict node_id to (0, 32K).
 If this is a problem, better correct it as soon as possible.
*/

#define ADQ_NONE	0x00
#define ADQ_LOCAL	0x80
#define ADQ_MASTER	0x40
#define ADQ_MSG		0xC0
#define ADQ_OSS 	0x20

#define CMD_INFO	0x01
#define CMD_SET		0x02
#define CMD_MASTER	0x03
#define CMD_DISP	0x04
#define CMD_TRACE	0x05
#define CMD_BIND	0x06
#define CMD_SLEEP	0x07
// CMD_BIND, CMD_SLEEP are not implemented yet

// for documenting only: from enum in msg_rtag.h
// (msg_bind, msg_sleep are not implemented yet)
#if 0
#define MSG_MASTER	0x01
#define MSG_TRACE	0x02
#define MSG_TRACEACK	0x03
#define MSG_CMD		0x04
#define MSG_BIND	0x05
#define MSG_SLEEP	0x06
#endif

#define RC_NONE		0xFF
#define RC_OK		0
#define RC_OKRET	1
#define RC_ERES		2
#define RC_ENET		3
#define RC_EMAS		4
#define RC_ECMD		5
#define RC_EPAR		6
#define RC_EVAL		7
#define RC_ELEN		8
#define RC_EADDR	9
#define RC_EFORK	10
#define RC_EMEM		11

#define PAR_ERR 	0
#define PAR_LH		1
//#define PAR_HID		2
//#define PAR_MID		3
#define PAR_NID		4
#define PAR_TARP_L	5
#define PAR_TARP_S	6
#define PAR_TARP_R	7
#define PAR_TARP_F	11
#define PAR_BEAC	8
#define PAR_PIN		9
#define PAR_RF		10

/* The "groups" for CMD_INFO retrievals:
SYS: host_id, local_host, net_id, seconds(),  master_host, master_delta,
     beac_freq

MSG: txrx, tarp {l, s, n}, count {r_a, r_t, s_a, s_t, f_a, f_t}

MEM: free, min, max_block, chunks, stack, static

PIN: all pins 1-11

ADC: sampling (given rxxxxppp, stime, where r is ref, ppp are pins)
     (in the future: see the documentation)

UART: rate, bits, parity, flow

*/
#define INFO_ERR	0
#define INFO_SYS	1
#define INFO_MSG	2
#define INFO_MEM	3
#define INFO_PIN	4
#define INFO_ADC	5
#define INFO_UART	6
#endif
