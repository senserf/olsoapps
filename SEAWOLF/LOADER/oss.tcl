#!/bin/sh
###########################\
exec tclsh "$0" "$@"
#
# An OSS script to talk to the application
#
set Debug 		1
set MyESN		0xBACA0001
set MyLink		[expr $MyESN & 0x0000ffff]
set YourESN		0
set YourLink		0
set MyRQN		255
set YourRQN		0
set MaxLineCount	1024
set CHUNKSIZE		56
set ICHUNKSIZE		54

array set CCOD 		{ DBG 0 HELLO 1 OSS 2 GO 3 WTR 4 RTS 5 CHUNK 6 ACK 7
				STAT 8 }

array set OSSC		{ PING 0 GET 1 QUERY 2 CLEAN 3 SHOW 4 LCDP 5 BUZZ 6
				RFPAR 7 DUMP 8 EE 9 }

array set ACKC		{ OK 0 FAILED 1 BUSY 2 REJECT 3 NOTFOUND 4 FORMAT 5
				ALREADY 6 UNIMPL 7 }

array set INTV		{ PACKET 50 RETRY 1000 CHUNK 2048 SPACE 80 GO 4096 }

set CHAR(PRE)		[format %c [expr 0x55]]
set CHAR(0)		\x00

# user commands
set UCMDS \
"verbose retrieve getobject ping clean query show lcd buzz rfparam dump erase quit"


set SIO(VER) 1
set SIO(ADV) 0


# ISO 3309 CRC table
set CRCTAB	{
    0x0000  0x1021  0x2042  0x3063  0x4084  0x50a5  0x60c6  0x70e7
    0x8108  0x9129  0xa14a  0xb16b  0xc18c  0xd1ad  0xe1ce  0xf1ef
    0x1231  0x0210  0x3273  0x2252  0x52b5  0x4294  0x72f7  0x62d6
    0x9339  0x8318  0xb37b  0xa35a  0xd3bd  0xc39c  0xf3ff  0xe3de
    0x2462  0x3443  0x0420  0x1401  0x64e6  0x74c7  0x44a4  0x5485
    0xa56a  0xb54b  0x8528  0x9509  0xe5ee  0xf5cf  0xc5ac  0xd58d
    0x3653  0x2672  0x1611  0x0630  0x76d7  0x66f6  0x5695  0x46b4
    0xb75b  0xa77a  0x9719  0x8738  0xf7df  0xe7fe  0xd79d  0xc7bc
    0x48c4  0x58e5  0x6886  0x78a7  0x0840  0x1861  0x2802  0x3823
    0xc9cc  0xd9ed  0xe98e  0xf9af  0x8948  0x9969  0xa90a  0xb92b
    0x5af5  0x4ad4  0x7ab7  0x6a96  0x1a71  0x0a50  0x3a33  0x2a12
    0xdbfd  0xcbdc  0xfbbf  0xeb9e  0x9b79  0x8b58  0xbb3b  0xab1a
    0x6ca6  0x7c87  0x4ce4  0x5cc5  0x2c22  0x3c03  0x0c60  0x1c41
    0xedae  0xfd8f  0xcdec  0xddcd  0xad2a  0xbd0b  0x8d68  0x9d49
    0x7e97  0x6eb6  0x5ed5  0x4ef4  0x3e13  0x2e32  0x1e51  0x0e70
    0xff9f  0xefbe  0xdfdd  0xcffc  0xbf1b  0xaf3a  0x9f59  0x8f78
    0x9188  0x81a9  0xb1ca  0xa1eb  0xd10c  0xc12d  0xf14e  0xe16f
    0x1080  0x00a1  0x30c2  0x20e3  0x5004  0x4025  0x7046  0x6067
    0x83b9  0x9398  0xa3fb  0xb3da  0xc33d  0xd31c  0xe37f  0xf35e
    0x02b1  0x1290  0x22f3  0x32d2  0x4235  0x5214  0x6277  0x7256
    0xb5ea  0xa5cb  0x95a8  0x8589  0xf56e  0xe54f  0xd52c  0xc50d
    0x34e2  0x24c3  0x14a0  0x0481  0x7466  0x6447  0x5424  0x4405
    0xa7db  0xb7fa  0x8799  0x97b8  0xe75f  0xf77e  0xc71d  0xd73c
    0x26d3  0x36f2  0x0691  0x16b0  0x6657  0x7676  0x4615  0x5634
    0xd94c  0xc96d  0xf90e  0xe92f  0x99c8  0x89e9  0xb98a  0xa9ab
    0x5844  0x4865  0x7806  0x6827  0x18c0  0x08e1  0x3882  0x28a3
    0xcb7d  0xdb5c  0xeb3f  0xfb1e  0x8bf9  0x9bd8  0xabbb  0xbb9a
    0x4a75  0x5a54  0x6a37  0x7a16  0x0af1  0x1ad0  0x2ab3  0x3a92
    0xfd2e  0xed0f  0xdd6c  0xcd4d  0xbdaa  0xad8b  0x9de8  0x8dc9
    0x7c26  0x6c07  0x5c64  0x4c45  0x3ca2  0x2c83  0x1ce0  0x0cc1
    0xef1f  0xff3e  0xcf5d  0xdf7c  0xaf9b  0xbfba  0x8fd9  0x9ff8
    0x6e17  0x7e36  0x4e55  0x5e74  0x2e93  0x3eb2  0x0ed1  0x1ef0
}

proc dump { hdr buf } {

	global Debug

	if $Debug {
		set code ""
		set nb [string length $buf]
		binary scan $buf c$nb code
		set ol ""
		foreach co $code {
			append ol [format " %02x" [expr $co & 0xff]]
		}
		log "$hdr:$ol"
	}
}
		
proc w_chk { wa } {

	global CRCTAB

	set nb [string length $wa]

	set chs 0

	while { $nb > 0 } {

		binary scan $wa s waw
		set waw [expr $waw & 0x0000ffff]

		set wa [string range $wa 2 end]
		incr nb -2

		set chs [expr (($chs << 8) ^ \
		    ( [lindex $CRCTAB [expr ($chs >> 8) ^ ($waw >>   8)]] )) & \
			0x0000ffff ]
		set chs [expr (($chs << 8) ^ \
		    ( [lindex $CRCTAB [expr ($chs >> 8) ^ ($waw & 0xff)]] )) & \
			0x0000ffff ]
	}

	return $chs
}

proc listen { } {
#
# Initialize serial I/O
#
	global SFD SIO

	fconfigure $SFD -mode "115200,n,8,1" -handshake none \
		-buffering full -translation binary -blocking 0

	# initialize serial status
	set SIO(TIM)  ""
	set SIO(STA) -1

	fileevent $SFD readable r_serial

	fconfigure stdin -buffering line -blocking 0
	
	fileevent stdin readable r_user

	set_handler HELLO handle_hello

	vwait SFD
#
# When SFD is set (to "") we terminate
#
	exit 0
}

proc put1 { v } {

	global SIO

	append SIO(MSG) [binary format c $v]
}

proc put2 { v } {

	global SIO

	append SIO(MSG) [binary format s $v]
}

proc put4 { v } {

	global SIO

	append SIO(MSG) [binary format i $v]
}

proc get1 { } {

	global SIO

	if { $SIO(BUF) == "" } {
		# just in case
		return 0
	}

	binary scan $SIO(BUF) c b

	set SIO(BUF) [string range $SIO(BUF) 1 end]

	return [expr $b & 0xff]
}

proc get2 { } {

	global SIO

	set w 0

	binary scan $SIO(BUF) s w

	set SIO(BUF) [string range $SIO(BUF) 2 end]

	return [expr $w & 0xffff]
}

proc get4 { } {

	global SIO

	set d 0
	binary scan $SIO(BUF) i d
	set SIO(BUF) [string range $SIO(BUF) 4 end]

	return $d
}

proc set_handler { pt { fun "" } } {
#
# Set a handler for incoming packets
#
	global CCOD CMD

	if ![info exists CCOD($pt)] {
		error "illegal packet type for set_handler"
	}

	set CMD($CCOD($pt)) $fun
}

proc clear_handlers { } {
#
# Remove all incoming packet handlers except HELLO
#
	global CCOD CMD

	foreach nm [array names CCOD] {
		if { $nm == "HELLO" } {
			continue
		}
		set CMD($CCOD($nm)) ""
	}
}

proc init_msg { lid } {

	global SIO

	set SIO(MSG) [binary format s $lid]
}

proc msg_close { } {

	global SIO CHAR

	set ln [string length $SIO(MSG)]

	if [expr $ln & 0x01] {
		append SIO(MSG) $CHAR(0)
	}

	set chs [w_chk $SIO(MSG)]
	append SIO(MSG) [binary format s $chs]
}

proc r_serial { } {

	global SFD SIO CHAR INTV
#
#  STA = -1  == Waiting for preamble
#  STA >  0  == Waiting for STA bytes to end of packet
#  STA =  0  == Waiting for the length byte
#
	set chunk ""

	while 1 {

		if { $chunk == "" } {

			if [catch { read $SFD } chunk] {
				# ignore errors
				set chunk ""
			}
				
			if { $chunk == "" } {
				# check if the timeout flag is set
				if { $SIO(TIM) != "" } {
					if { $SIO(VER) > 0 } {
						log "packet timeout $SIO(STA)"
					}
					# reset
					catch { after cancel $SIO(TIM) }
					set SIO(TIM) ""
					set SIO(STA) -1
				} elseif { $SIO(STA) > -1 } {
					# packet started, set timeout
					set SIO(TIM) \
						[after $INTV(PACKET) r_serial]
				}
				return
			}
			if { $SIO(TIM) != "" } {
				catch { after cancel $SIO(TIM) }
				set SIO(TIM) ""
			}
		}

		set bl [string length $chunk]

		if { $SIO(STA) > 0 } {

			# waiting for the balance
			if { $bl < $SIO(STA) } {
				append SIO(BUF) $chunk
				set chunk ""
				incr SIO(STA) -$bl
				continue
			}

			if { $bl == $SIO(STA) } {
				append SIO(BUF) $chunk
				set chunk ""
				# we have a complete buffer
				runit
				set SIO(STA) -1
				continue
			}

			# merged packets
			append SIO(BUF) [string range $chunk 0 \
				[expr $SIO(STA) - 1]]
			set chunk [string range $chunk $SIO(STA) end]
			runit
			set SIO(STA) -1
			continue
		}

		if { $SIO(STA) == -1 } {
			# waiting for preamble
			for { set i 0 } { $i < $bl } { incr i } {
				if { [string index $chunk $i] == $CHAR(PRE) } {
					# preamble found
					break
				}
			}
			if { $i == $bl } {
				# not found
				set chunk ""
				continue
			}
			set SIO(STA) 0
			incr i
			set chunk [string range $chunk $i end]
			continue
		}

		# now for the length byte
		binary scan [string index $chunk 0] c bl
		set chunk [string range $chunk 1 end]
		if { $bl <= 0 || [expr $bl & 0x01] } {
			# illegal
			if { $SIO(VER) > 0 } {
				log "illegal packet length $bl"
			}
			set SIO(STA) -1
			continue
		}

		set SIO(STA) [expr $bl + 4]
		set SIO(BUF) ""
	}
}

proc w_serial { } {

	global SFD SIO CHAR

	set ln [expr [string length $SIO(MSG)] - 4]

	puts -nonewline $SFD $CHAR(PRE)
	puts -nonewline $SFD [binary format c $ln]
	puts -nonewline $SFD $SIO(MSG)

	flush $SFD

	if { $SIO(VER) > 2 } {
		dump "S" $SIO(MSG)
	}
}

proc handle_debug { } {

	global SIO CHAR

	set fg [get1]

	if { $fg == 0 } {
		# textual debug
		set a [get2]
		set b [get2]
		set ix [string first $CHAR(0) $SIO(BUF)]
		if { $ix < 0 } {
			log "unterminated string in debug packet"
		}
		incr ix -1
		log "DEBUG: [string range $SIO(BUF) 0 $ix] <$a,$b>"
		return
	}

	if { $fg == 1 } {
		log "MEMORY DUMP:"
	} else {
		log "EEPROM DUMP:"
	}

	set code ""
	set nb [expr [string length $SIO(BUF)] - 2]
	binary scan $SIO(BUF) c$nb code
	set ol ""
	foreach co $code {
		append ol [format "%02x " [expr $co & 0xff]]
	}
	log "$ol"
}
	
proc runit { } {

	global SIO CMD CCOD

	if { $SIO(VER) > 2 } {
		# dump the packet
		dump "R" $SIO(BUF)
	}

	# validate CRC
	if [w_chk $SIO(BUF)] {
		if { $SIO(VER) > 0 } {
			log "illegal checksum, packet ignored"
		}
		return
	}
	
	# extract the link Id and command
	binary scan $SIO(BUF) sc ld cmd
	set SIO(LID) [expr $ld & 0x0000ffff]

	# strip off the command byte and checksum
	set SIO(BUF) [string range $SIO(BUF) 3 end-2]

	if { $cmd == $CCOD(DBG) } {
		handle_debug
		return
	}

	if ![info exists CMD($cmd)] {
		if { $cmd == $CCOD(ACK) } {
			handle_ack
			return
		}
		if { $SIO(VER) > 0 } {
			log "illegal packet type $cmd, ignored"
		}
		return
	}

	if { $CMD($cmd) == "" } {
		if { $SIO(VER) > 1 } {
			log "unsolicited packet $cmd, ignored"
		}
		return
	}

	# execute the handler
	$CMD($cmd)
}

proc r_user { } {
#
# Input user command
#
	global SFD UCMDS

	if [catch { gets stdin line } stat] {
		# ignore any errors (can they happen at all?)
		return
	}

	if { $stat < 0 } {
		# end of file
		terminate
		return
	}

	set line [string trim $line]

	if { $line == "" } {
		# always quietly ignore empty lines
		return
	}

	if ![regexp -nocase "^(\[a-z\]+)(.*)" $line junk kwd line] {
		log "illegal user input"
		return
	}

	# the arguments
	set line [string trimleft $line]
	set kwd "^$kwd"

	foreach c $UCMDS {
		if [regexp $kwd $c] {
			user_$c $line
			return
		}
	}

	log "unknown command"
}

proc handle_hello { } {
#
# Received HELLO
#
	global SIO YourESN YourLink

	# ignore the rqnum field
	get1

	# this is the ESN
	set esn [get4]
	if { $esn == 0 } {
		log "received illegal ESN of 0"
		return
	}

	if { $YourESN == 0 } {
		# first time
		log "received ESN: [format %08x $esn]"
	} elseif { $esn != $YourESN } {
		log "ESN changed: [format %08x $YourESN] -> [format %08x $esn]"
	}
	set YourESN $esn
	set YourLink [expr $YourESN & 0x0000ffff]
}

proc user_verbose { par } {
#
# Dumping on/off
#
	global SIO

	if [catch { expr int($par) } par] {
		log "illegal numeric parameter"
		return
	}

	set SIO(VER) $par
}

proc exnum { s } {
#
# extracts a number from the string
#
	upvar $s str

	set str [string trimleft $str]

	if ![regexp "^(\[^ \t\]+)(.*)" $str junk num str] {
		return ""
	}

	if [catch { expr int($num) } num] {
		return ""
	}

	return $num
}

proc poll { } {
#
# Retransmit a packet
#
	global SIO

	if { $SIO(TRY) == 0 } {
		# no more
		set SIO(ADV) 1
		unset SIO(MSG)
		return
	}

	incr SIO(TRY) -1

	w_serial

	set SIO(CBA) [after $SIO(INT) poll]
}

proc abtreq { code } {
#
# Abort current request, e.g., on a negative ACK
#
	global SIO

	catch { after cancel $SIO(CBA) }
	catch { unset SIO(MSG) }
	set SIO(ADV) [expr $code + 128]
}

proc advreq { } {
#
# Advance current request
#
	global SIO

	catch { after cancel $SIO(CBA) }
	catch { unset SIO(MSG) }
	set SIO(ADV) 0
}

proc reset_chunk_timer { } {
#
# Trigger timeout if a chunks doesn't arrive soon enough
#
	global SIO INTV

	catch { after cancel $SIO(CBA) }
	set SIO(CBA) [after $INTV(CHUNK) chunk_timeout]
}

proc chunk_timeout { } {

	set SIO(ADV) 0
}

proc go_timeout { } {

	set SIO(ADV) 127
}

proc retrans { intv count } {
#
# Retransmits the packet count times at intv intervals, or until a reply is
# received
#
	global SIO

	# the first time
	w_serial

	# attempt counter
	set SIO(TRY) $count

	# the interval
	set SIO(INT) $intv

	# timer callback
	set SIO(CBA) [after $intv poll]

	# wait until resolved: this will be set to success or failure after
	# this step of the handshake has been accomplished
	vwait SIO(ADV)
}

proc inirqm { pt } {
#
# Initialize a request message
#
	global CCOD MyLink MyRQN YourLink

	incr MyRQN
	if { $MyRQN > 255 } {
		# request number
		set MyRQN 1
	}

	init_msg $MyLink
	put1 $CCOD($pt)
	put1 $MyRQN
	put2 $YourLink
}

proc user_retrieve { par } {
#
# Retrieve an object from the node
#
	global SIO CCOD CHUNKS YourLink MyLink MyRQN

	if { $YourLink == 0 } {
		log "don't know about the node yet"
		return
	}

	# two numbers: object type + Id
	set SIO(OTP) [exnum par]
	set SIO(OID) [exnum par]

	# file name for retrieved image
	set SIO(FIL) [string trim $par]

	if { $SIO(OTP) == "" || $SIO(OID) == "" } {
		log "illegal object parameters"
		return
	}

	inirqm WTR
	put2 $SIO(OTP)
	put2 $SIO(OID)
	msg_close

	# save for retransmissions
	set_handler RTS handle_rts
	set_handler ACK handle_wtr_ack

	# retransmissions: interval, how many times
	retrans 2048 8

	set_handler RTS

	if $SIO(ADV) {
		log "request denied, code $SIO(ADV)"
		clear_handlers
		return
	}

	# received RTS, proceed to next stage; this is the total number of
	# chunks to be received
	set nc [lindex $SIO(RTS) 0]

	for { set ic 0 } { $ic < $nc } { incr ic } {
		set CHUNKS($ic) ""
	}

	# the number of chunks to be received
	set SIO(NCK) $nc
	# remaining chunks
	set SIO(RCK) $nc

	set_handler CHUNK handle_chunk

	while { [go_prompt] } {

		retrans 512 16

		if $SIO(ADV) {
			log "request interrupted, code $SIO(ADV)"
			clear_handlers
			array unset CHUNKS
			return
		}
	}

	# all chunks received, send the final stop packet

	init_msg $MyLink
	put1 $CCOD(GO)
	put1 $MyRQN
	msg_close
	w_serial
	after 256
	w_serial
	unset SIO(MSG)

	# combine all the chunks into a single block of data
	set chk ""
	for { set ic 0 } { $ic < $nc } { incr ic } {
		append chk $CHUNKS($ic)
	}

	array unset CHUNKS
	clear_handlers

	switch $SIO(OTP) {

		0 { receive_image $chk $SIO(OID) }
		1 { receive_ilist $chk $SIO(OID) }
		2 { receive_nlist $chk $SIO(OID) }

		default {
			log "received unknown type ($SIO(OTP)) object\
				number $SIO(OID), length [string length $chk]"
		}
	}
}
		
proc go_prompt { } {
#
# Prepare a GO packet based on the list of chunks that are still missing
#
	global SIO CHUNKS CCOD MyRQN MyLink

	if { $SIO(RCK) == 0 } {
		# all done
		return 0
	}

	init_msg $MyLink
	put1 $CCOD(GO)
	put1 $MyRQN

	set n 0

	set i 0
	set l $SIO(NCK)

	while { $i != $l } {
		# opening a new range
		if { $CHUNKS($i) != "" } {
			incr i
			continue
		}

		if { $n == 22 } {
			# only one left
			put2 $i
			break
		}

		set s $i
		incr i

		while { $i != $l } {
			if { $CHUNKS($i) != "" } {
				break
			}
			incr i
		}

		set t [expr $i - 1]
		if { $s == $t } {
			# a singleton
			put2 $s
			incr n
			continue
		}

		# a range
		put2 $t
		put2 $s

		incr n 2

		if { $n == 23 } {
			break
		}
	}

	if { $n == 0 } {
		error "illegal go_prompt, no missing chunks"
	}

	msg_close

	return 1
}

proc handle_wtr_ack { } {
#
# Handle a negative ACK to our WTR
#
	global SIO MyRQN MyLink

	if { $SIO(LID) != $MyLink } {
		# ignore, although perhaps shouldn't
		return
	}

	set rq [get1]
	if { $rq != $MyRQN } {
		# obsolete, irrelevant
		return
	}

	# applies to my request
	set rq [get2]

	# the code must be nonzero (zero is OK)
	if { $rq == 0 } {
		# ignore anyway
		return
	}

	if { $SIO(VER) > 0 } {
		log "negative ACK [ack_code $rq]"
	}

	abtreq $rq
}

proc handle_rts { } {
#
# Receive RTS
#
	global SIO MyRQN MyLink CHAR

	if { $SIO(LID) != $MyLink } {
		return
	}

	set rq [get1]
	if { $rq != $MyRQN } {
		return
	}

	# the rest depends on the object type

	switch $SIO(OTP) {

		0 {
			set na [get2]
			set nb [get2]
			set nc [get2]
			set nd [get2]
			set ix [string first $CHAR(0) $SIO(BUF)]
			if { $ix >= 0 } {
				set lab [string range $SIO(BUF) 0 [expr $ix-1]]
			} else {
				set lab [string range $SIO(BUF) 0 end-1]
			}
			set SIO(RTS) [list $na $nb $nc $nd $lab]
		}

		default {
			set na [get2]
			set nb [get2]

			set SIO(RTS) [list $na $nb]
		}
	}

	# advance the request
	advreq
}

proc handle_chunk { } {
#
# Receive a chunk packet
#
	global SIO CHUNKS MyRQN MyLink

	if { $SIO(LID) != $MyLink } {
		return
	}

	set rq [get1]
	if { $rq != $MyRQN } {
		return
	}

	reset_chunk_timer

	if { [string length $SIO(BUF)] < 3 } {
		# empty chunk, end of round
		advreq
		return
	}

	# chunk number
	set rq [get2]

	if { $rq >= $SIO(NCK) } {
		# illegal
		if { $SIO(VER) > 0 } {
			log "received illegal chunk number $rq / $SIO(NCK)"
		}
		abtreq 12
		return
	}

	if { $CHUNKS($rq) == "" } {
		set CHUNKS($rq) $SIO(BUF)
		incr SIO(RCK) -1
		if { $SIO(VER) > 1 } {
			log "received chunk $rq ([string length $SIO(BUF)])"
		}
	} else {
		if { $SIO(VER) > 1 } {
			log "received superfluous chunk $rq"
		}
	}
}

proc user_getobject { par } {
#
# Tell the node to get an object
#
	global SIO INTV CCOD OSSC CHUNKS REQCK MyLink YourLink YourRQN

	if { $YourLink == 0 } {
		log "don't know about the node yet"
		return
	}

	set lnk [exnum par]
	set otp [exnum par]
	set oid [exnum par]
	set rqf [exnum par]

	if { $lnk == "" || $otp == "" || $oid == "" || $rqf == "" } {
		log "numeric input expected: lk ot oid rqf \[fname\]"
		return
	}

	if { $lnk == $MyLink } {
		# over UART
		set ifc 1
		if { $otp != 0 } {
			log "we only send images (ot must be zero)"
			return
		}
		# expect a file name
		set par [string trim $par]
		if { [set_image $oid $par] == 0 } {
			# problem diagnosed by the function
			return
		}
	} else {
		# over radio
		set ifc 0
	}

	inirqm OSS

	put1 $OSSC(GET)
	# this one will be ignored
	put1 $OSSC(GET)

	put2 $lnk
	put2 $otp
	put2 $oid
	put1 $ifc
	put1 $rqf
	msg_close

	if { $ifc == 0 } {
		# send it once
		w_serial
		return
	}

	# sending own image
	set_handler WTR handle_wtr
	set_handler ACK handle_get_ack

	# keep retransmitting until WTR or OSS BUSY
	retrans 2048 8

	if $SIO(ADV) {
		log "request denied, code $SIO(ADV)"
		clear_handlers
		clear_image
		return
	}

	# next stage, in response to WTR, send RTS
	clear_handlers

	init_msg $YourLink
	put1 $CCOD(RTS)
	put1 $YourRQN

	# set up by set_image
	put2 [lindex $SIO(RTS) 0]
	put2 [lindex $SIO(RTS) 1]
	put2 [lindex $SIO(RTS) 2]
	put2 [lindex $SIO(RTS) 3]
	# the label
	append SIO(MSG) [lindex $SIO(RTS) 4]
	msg_close

	# transmit chunks

	set_handler GO handle_go
	set_handler ACK handle_rts_ack

	retrans 2048 8

	if $SIO(ADV) {
		log "request denied after RTS, code $SIO(ADV)"
		clear_handlers
		clear_image
		return
	}

	while 1 {
		clear_handlers
		# we have received a GO
		if { $SIO(RCK) == 0 } {
			# all done
			break
		}
		set cp 0
		while { $cp < $SIO(RCK) } {
			set sp $REQCK($cp)
			incr cp
			if { $cp >= $SIO(RCK) || $REQCK($cp) > $sp } {
				set fp $sp
			} else {
				set fp $REQCK($cp)
				incr cp
			}
			while { $fp <= $sp } {
				if { $SIO(VER) > 1 } {
					log "sending chunk: $fp"
				}
				init_msg $YourLink
				put1 $CCOD(CHUNK)
				put1 $YourRQN
				put2 $fp
				append SIO(MSG) $CHUNKS($fp)
				msg_close
				w_serial
				after $INTV(SPACE)
				incr fp
			}
		}

		set_handler GO handle_go

		# send an empty chunk
		init_msg $YourLink
		put1 $CCOD(CHUNK)
		put1 $YourRQN
		msg_close
		
		retrans 512 8

		if $SIO(ADV) {
			if { $SIO(VER) > 0 } {
				log "GO timeout, code $SIO(ADV)"
			}
			clear_handlers
			clear_image
			return
		}
	}

	log "transaction complete"
	clear_handlers
	clear_image
}

proc handle_rts_ack { } {
#
# Negative ACK after RTS
#
	global SIO YourRQN YourLink

	if { $SIO(LID) != $YourLink } {
		# ignore
		return
	}

	set rq [get1]
	if { $rq != $YourRQN } {
		return
	}

	set rq [get2]

	if { $rq == 0 } {
		return
	}

	if { $SIO(VER) > 0 } {
		log "NAK [ack_code $rq]"
	}

	abtreq $rq
}

proc handle_go { } {
#
# Unpack the requested list of chunks
#
	global SIO REQCK YourRQN YourLink

	if { $SIO(LID) != $YourLink } {
		return
	}

	set rq [get1]
	if { $rq != $YourRQN } {
		abtreq 15
		return
	}

	set ln [expr [string length $SIO(BUF)] / 2]
	set SIO(RCK) $ln

	set cn 0

	while { $cn < $ln } {
		set ci [get2]
		set REQCK($cn) $ci
		incr cn
	}

	advreq
}

proc handle_wtr { } {
#
# WTR prompt (in response to OSS GET)
#
	global SIO YourRQN YourLink MyLink

	if { $SIO(LID) != $YourLink } {
		abtreq 13
		return
	}

	set YourRQN [get1]
	set ml [get2]
	if { $ml != $MyLink } {
		abtreq 14
		return
	}

	set ml [get2]
	set mn [get2]
	# must be image, we send out nothing else
	if { $ml != 0 } {
		if { $SIO(VER) > 0 } {
			log "illegal WTR request: $ml $mn"
		}
		abtreq 15
		return
	}

	# OK, we can proceed
	advreq
}

proc handle_get_ack { } {
#
# ACK response to our GET
#
	global SIO MyRQN

	set rq [get1]

	if { $rq != $MyRQN } {
		return
	}

	# this must be negative

	set wc [get2]

	if { $wc == 0 } {
		return
	}

	if { $SIO(VER) > 0 } {
		log "negative ACK [ack_code $wc] in response to GET"
	}

	abtreq $wc
}

proc geti1 { } {

	global SIO IDX

	set B [string index $SIO(IB) $IDX]

	if { $B == "" } {
		# just in case
		return 0
	}

	incr IDX

	binary scan $B c b
	return [expr $b & 0xff]
}

proc geti2 { } {

	global SIO IDX

	set B [string index $SIO(IB) $IDX]
	if { $B == "" } {
		return 0
	}
	incr IDX
	set C [string index $SIO(IB) $IDX]
	if { $C == "" } {
		return 0
	}
	incr IDX

	binary scan $B$C s w
	return [expr $w & 0xffff]
}

proc geti4 { } {

	global SIO IDX

	set B [string index $SIO(IB) $IDX]
	if { $B == "" } {
		return 0
	}
	incr IDX
	set C [string index $SIO(IB) $IDX]
	if { $C == "" } {
		return 0
	}
	incr IDX
	set D [string index $SIO(IB) $IDX]
	if { $D == "" } {
		return 0
	}
	incr IDX
	set E [string index $SIO(IB) $IDX]
	if { $E == "" } {
		return 0
	}
	incr IDX

	binary scan $B$C$D$E i d

	return $d
}

proc getis { n } {

	global SIO IDX IDL CHAR

	set la [expr $IDX + $n]
	set le [expr $la - 1]

	set B [string range $SIO(IB) $IDX $le]

	if { $la > $IDL } {
		for { set i $IDL } { $i < $la } { incr i } {
			append B $CHAR(0)
		}
		set IDX $IDL
	} else {
		set IDX $la
	}
	return $B
}

proc set_image { oid fn } {
#
# Unpack image to be sent out
#
	global SIO IDX IDL CHUNKS ICHUNKSIZE CHAR

	if [catch { open $fn "r" } ifd] {
		log "cannot open file $fn"
		return 0
	}

	fconfigure $ifd -translation binary

	if [catch { read $ifd } SIO(IB)] {
		log "cannot read file $fn"
		catch { close $ifd }
		unset SIO(IB)
		return 0
	}

	catch { close $ifd }

	set IDL [string length $SIO(IB)]
	log "image file $fn read, length $IDL bytes"

	if { [string range $SIO(IB) 0 3] != "olso" } {
		log "illegal image header in file $fn"
		unset SIO(IB)
		return 0
	}

	set IDX 4

	set bpp [geti2]
	set x [geti2]
	set y [geti2]

	if $bpp {
		set pxl 8
	} else {
		set pxl 12
	}

	log "geometry: <$x,$y>, $pxl bpp"

	set l  [geti2]
	# the label
	if $l {
		set la [getis $l]
		log "label: $la"
	} else {
		set la ""
		log "no label"
	}

	# label sentinel
	append la $CHAR(0)
	incr l

	if { $l > 50 } {
		set la [string range $la 0 49]
	} elseif { [expr $l & 1] != 0 } {
		# make sure the length is even
		append la $CHAR(0)
	}

	set nc 0
	while 1 {
		set CHUNKS($nc) [getis $ICHUNKSIZE]
		incr nc
		if { $IDX >= $IDL } {
			break
		}
	}

	log "$nc chunks total"
	unset SIO(IB) IDX IDL

	if $bpp {
		set x [expr $x | 0x8000]
	}

	set SIO(RTS) [list $nc $oid $x $y $la]

	return 1
}

proc clear_image { } {
#
	global SIO CHUNKS REQCK

	array unset CHUNKS
	array unset REQCK
}

proc receive_image { chk id } {
#
# Received an image
#
	global SIO CHUNKSIZE

	if { $SIO(FIL) == "" } {
		log "no file name specified, image discarded"
		return
	}

	# will built the image file here
	set SIO(MSG) "olso"

	set nc [lindex $SIO(RTS) 0]
	# image number ignored
	set xx [lindex $SIO(RTS) 2]
	set yy [lindex $SIO(RTS) 3]
	set ll [lindex $SIO(RTS) 4]

	if { [expr $xx & 0x800] != 0 } {
		put2 1
	} else {
		put2 0
	}

	put2 [expr $xx & ~0x800]
	put2 [string length $ll]
	append SIO(MSG) $ll

	# re-order the chunks
	for { set i 0 } { $i < $nc } { incr i } {
		set cc($i) ""
	}

	for { set j 0 } { $j < $nc } { incr j } {
		set fr [expr $i * $CHUNKSIZE]
		set up [expr $fr + $CHUNKSIZE - 1]
		set ex [string range $chk $fr $up]
		set i [string range $ex 0 1]
		set ex [string range $ex 2 end]

		binary scan $i s i
		set i [expr $i & 0xffff]
		# chunk number
		if { $i >= $nc } {
			log "error collecting chunks 1"
			return
		}

		if { $cc($i) != "" } {
			log "error collecting chunks 2"
			return
		}

		set cc($i) $ex
	}

	for { set i 0 } { $i < $nc } { incr i } {
		if { $cc($i) == "" } {
			log "error collecting chunks 3"
			return
		}
		append SIO(MSG) $cc($i)
	}

	# write it out
			
	if [catch { open $SIO(FIL) "w" } fd] {
		log "cannot open $SIO(FIL): $fd"
		return
	}

	if [catch { puts -nonewline $fd $SIO(MSG) } err] {
		log "cannot write to $SIO(FIL): $err"
		catch { close $fd }
		return
	}

	catch { close $fd }

	log "image saved in $SIO(FIL)"
}

proc receive_ilist { chk oid } {
#
# Image list
#
	global SIO IDX IDL CHAR

	set SIO(IB) $chk
	set IDX	0
	set IDL [string length $chk]

	set id [geti2]

	log "image list $oid, owner link id: $id, [format %04x $id]"

	while { $IDX < $IDL } {
		set nx [geti2]
		set fi [geti2]
		set si [geti2]
		set x  [geti2]
		set y  [geti2]
		set ls [expr $nx - 8]
		if { $ls > 0 } {
			set la [getis $ls]
			set ix [string first $CHAR(0) $la]
			if { $ix >= 0 } {
				incr ix -1
				set la [string range $la 0 $ix]
			}
		} else {
			set la ""
		}
		if { $y == 0 } {
			log "the image list is truncated"
			unset SIO(IB)
			return
		}
		if [expr $x & 0x8000] {
			set bpp 8
		} else {
			set bpp 12
		}
		set x [expr $x & ~0x8000]
		log " image $fi, size $si chunks, $x x $y ($bpp bpp)"
		if { $la != "" } {
			log " \"$la\""
		}
	}

	log "end of list"
	unset SIO(IB)
}

proc receive_nlist { chk oid }  {
#
	global SIO IDX IDL CHAR

	set SIO(IB) $chk
	set IDX	0
	set IDL [string length $chk]

	set id [geti2]

	log "Neighbor list $oid, owner link id: $id, [format %04x $id]"

	while { $IDX < $IDL } {

		set n [geti4]

		log "  -> [format %08x $n]"
	}

	unset SIO(IB)
	log "end of list"
}

proc user_ping { par } {
#
# Issue a ping
#
	global SIO CCOD OSSC YourLink

	if { $YourLink == 0 } {
		log "don't know about the node yet"
		return
	}

	inirqm OSS

	put1 $OSSC(PING)
	put1 $OSSC(PING)

	msg_close

	set_handler STAT handle_status

	w_serial
}

proc user_clean { par } {
#
# Clean request
#
	global SIO CCOD OSSC YourLink

	if { $YourLink == 0 } {
		log "don't know about the node yet"
		return
	}

	set otp [exnum par]

	if { $otp < 0 || $otp > 2 } {
		log "object type must be 0, 1, or 2"
		return
	}

	inirqm OSS
	put1 $OSSC(CLEAN)
	put1 $OSSC(CLEAN)
	put2 $otp

	set nob 0
	while 1 {
		set oid [exnum par]
		if { $oid == "" } {
			break
		}
		put2 $oid
		incr nob
		if { $nob == 27 } {
			break
		}
	}
	msg_close
	w_serial
}

proc user_query { par } {
#
# Object query
#
	global SIO CCOD OSSC YourLink

	if { $YourLink == 0 } {
		log "don't know about the node yet"
		return
	}

	set otp [exnum par]
	set oid [exnum par]

	if { $otp == "" || $oid == "" } {
		log "two numbers required: ot oid"
	}

	if { $otp < 0 || $otp > 2 } {
		log "object type must be 0, 1, or 2"
		return
	}

	inirqm OSS
	put1 $OSSC(QUERY)
	put1 $OSSC(QUERY)
	put2 $otp
	put2 $oid
	msg_close
	w_serial
}

proc user_show { par } {
#
# Clean request
#
	global SIO CCOD OSSC YourLink

	if { $YourLink == 0 } {
		log "don't know about the node yet"
		return
	}

	set img [exnum par]

	if { $img == "" } {
		log "one number expected: img"
		return
	}

	inirqm OSS
	put1 $OSSC(SHOW)
	put1 $OSSC(SHOW)
	put2 $img
	msg_close

	w_serial
}

proc user_lcd { par } {
#
# Issue an LCD command
#
	global SIO CCOD OSSC YourLink

	if { $YourLink == 0 } {
		log "don't know about the node yet"
		return
	}

	set cmd [exnum par]
	if { $cmd == "" } {
		log "expected syntax: cmd \[byte ... byte\]"
		return
	}

	set arg ""
	while 1 {
		set a [exnum par]
		if { $a == "" } {
			break
		}
		lappend arg $a
	}

	set ac [llength $arg]
	if { $ac > 48 } {
		set ac 48
	}

	inirqm OSS
	put1 $OSSC(LCDP)
	put1 $OSSC(LCDP)
	put1 $cmd
	put1 $ac

	for { set i 0 } { $i < $ac } { incr i } {
		put1 [lindex $arg $i]
	}
	msg_close

	w_serial
}

proc user_buzz { par } {
#
# Issue an LCD command
#
	global SIO CCOD OSSC YourLink

	if { $YourLink == 0 } {
		log "don't know about the node yet"
		return
	}

	set dur [exnum par]

	if { $dur == "" } {
		log "expected: dur"
		return
	}

	inirqm OSS
	put1 $OSSC(BUZZ)
	put1 $OSSC(BUZZ)
	put2 $dur
	msg_close

	w_serial
}

proc user_rfparam { par } {
#
# Issue an RF control command
#
	global SIO CCOD OSSC YourLink

	if { $YourLink == 0 } {
		log "don't know about the node yet"
		return
	}

	set cmd [exnum par]
	set val [exnum par]

	if { $cmd == "" || $val == "" } {
		log "two values expected: cmd val"
		return
	}

	inirqm OSS
	put1 $OSSC(RFPAR)
	put1 $OSSC(RFPAR)
	put1 $cmd
	put1 $val
	msg_close

	w_serial
}

proc user_dump { par } {
#
# Memory dump
#
	global SIO CCOD OSSC YourLink

	if { $YourLink == 0 } {
		log "don't know about the node yet"
		return
	}

	set ep 0

	if [regexp -nocase "^(\[a-z\]+)(.*)" $par junk ep par] {
		set par [string trimleft $par]
		if { [string tolower [string index $ep 0]] == "e" } {
			set ep 1
		} else {
			set ep 0
		}
	}

	set addr [exnum par]
	set count [exnum par]

	if { $addr == "" || $count == "" } {
		log "two numbers expected: addr count"
		return
	}

	if { $count < 2 } {
		set count 2
	} elseif { $count > 56 } {
		set count 56
	}

	inirqm OSS
	put1 $OSSC(DUMP)
	put1 $OSSC(DUMP)
	put1 $ep
	put1 $count
	put4 $addr
	msg_close

	w_serial
}

proc user_erase { par } {
#
# EEPROM erase
#
	global SIO CCOD OSSC YourLink

	if { $YourLink == 0 } {
		log "don't know about the node yet"
		return
	}

	set ep 0

	set from [exnum par]
	set upto [exnum par]

	if { $from == "" || $upto == "" } {
		log "two numbers expected: from upto"
		return
	}

	inirqm OSS
	put1 $OSSC(EE)
	put1 $OSSC(EE)
	put4 $from
	put4 $upto
	msg_close

	w_serial
}

proc ack_code { cd } {

	global ACKC

	foreach c [array names ACKC] {
		if { $ACKC($c) == $cd } {
			return $c
		}
	}

	return $cd
}

proc handle_status { } {
#
# Status packet
#
	get1

	log "STATUS:"
	set flg [get1]
	set nim [get1]
	set fre [get2]
	set rio [get2]
	set nec [get2]
	set rno [get2]
	if { $flg != 0 } {
		log " EEPROM inconsistency detected"
	}
	log " $nim images, $fre free pages"
	if { $rio != 0 } {
		log " acquired image list from $rio"
	}
	if { $nec != 0 } {
		log " $nec neighbors"
	}
	if { $rno != 0 } {
		log " acquired neighbor list from $rno"
	}
	return
}

proc handle_ack { } {
#
# Displays ACK packets arriving from the node in response to OSS commands
#
	global SIO

	set lnk $SIO(LID)
	set rqn [get1]
	set cod [get2]

	log "ACK [ack_code $cod] (rqn $rqn lnk [format %04x $lnk])"
}

proc user_quit { par } {
#
# Terminate
#
	global SFD

	catch { close $SFD }

	set SFD ""
}

proc log { m } {

	puts $m
	flush stdout
}

######### COM port ############################################################

set cpn [lindex $argv 0]
# puts $cpn

if { $cpn == "" } {
	# COM 1 is the default
	set cpn 1
} elseif { [catch { expr $cpn } ] || $cpn < 1 } {
	log "Illegal COM port number $cpn"
	after $INTV(ERRLINGER) "exit"
	set cpn ""
}

if { $cpn != ""  && [catch { open "com${cpn}:" "r+" } SFD] } {
	log "Cannot open COM${cpn}: $SFD"
	unset SFD
	exit 99
}

unset cpn

listen
