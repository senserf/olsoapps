/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#ifndef __msg_gene_h
#define __msg_gene_h
#include "msg_geneStructs.h"
typedef enum {
	msg_null, msg_master, msg_trace, msg_traceAck,
      	msg_cmd, msg_new, msg_bindReq, msg_bind, msg_br,
	msg_alrm, msg_st, msg_stAck, msg_stNack, msg_nh, msg_nhAck,
	msg_traceF, msg_traceFAck, msg_traceB, msg_traceBAck
} msgType;

#endif
