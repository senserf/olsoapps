#!/bin/sh
###########################\
exec tclsh "$0" "$@"
#
# An OSS script to talk to the application
#
set Debug 		1
set MyLink		1
set YourESN		0
set YourLink		0
set YourStat		0
set MyRQN		255
set YrRQN		0
set CHUNKSIZE		56
set ICHUNKSIZE		54
set EEPROMSIZE		[expr 256 * 2048]

set CHAR(PRE)		[format %c [expr 0x55]]
set CHAR(0)		\x00

array set CCOD 		{ GO 17 CHUNK 18 PING 240 SEND 241 RECV 242 ERAS 243 }
array set INTV		{ PACKET 100 CHUNK 2048 SPACE 10 ERRLINGER 2048 }
# user commands
set UCMDS "loss verbose dump load erase quit"

set LPR	"0.0"

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

	clear_handlers

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
					if { $SIO(VER) > 1 } {
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
# Remove all incoming packet handlers
#
	global CCOD CMD

	foreach nm [array names CCOD] {
		set CMD($CCOD($nm)) ""
	}
	set_handler PING standard_ping
}

proc standard_ping { } {
#
# Received PING
#
	global SIO YourESN YourLink YourStat MyRQN YrRQN

	set rqn [get1]
	set esn [get4]
	set sta [get2]

	set smsg "status = $sta / \[$MyRQN $rqn\]"

	if { $YourESN != $esn } {
		log "PING init ESN = [format %08x $esn], $smsg"
	} elseif { $YourStat != $sta || $YrRQN != $rqn } {
		log "PING $smsg"
	} elseif { $SIO(VER) > 1 } {
		log "PING (void) ESN = [format %08x $esn], $smsg"
	}
	set YourESN $esn
	set YrRQN $rqn
	set YourStat $sta
	set YourLink [expr $YourESN & 0x0000ffff]
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
	set cmd [expr $cmd & 0x000000ff]
	set SIO(LID) [expr $ld & 0x0000ffff]

	# strip off the command byte and checksum
	set SIO(BUF) [string range $SIO(BUF) 3 end-2]

	if { ![info exists CMD($cmd)] || $CMD($cmd) == "" } {
		if { $SIO(VER) > 1 } {
			log "unsolicited packet: $cmd"
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
	global UCMDS

	if [catch { gets stdin line } stat] {
		# ignore any errors (can they happen at all?)
		return
	}

	if { $stat < 0 } {
		# end of file
		exit 0
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

proc user_loss { par } {
#
# Set loss probability for send
#
	global LPR

	if [catch { expr double($par) } par] {
		log "illegal numeric parameter"
		return
	}

	if { $par < 0.0 || $par > 0.99 } {
		log "the argument must be between 0.0 and 0.99"
		return
	}

	set LPR $par
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

	if ![regexp "^(\[^ \t\]+)(.*)" $str junk num left] {
		return ""
	}

	if [catch { expr int($num) } num] {
		return ""
	}

	# remove the extracted piece
	set str $left

	return $num
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

proc chunk_timeout { } {

	global SIO

	set SIO(ADV) 0
}

proc reset_chunk_timer { } {
#
# Trigger timeout if a chunk doesn't arrive soon enough
#
	global SIO INTV

	catch { after cancel $SIO(CBA) }
	set SIO(CBA) [after $INTV(CHUNK) chunk_timeout]
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

proc abtreq { code } {
#
# Abort current request, e.g., on a negative ACK
#
	global SIO

	catch { after cancel $SIO(CBA) }
	catch { unset SIO(MSG) }
	set SIO(ADV) [expr $code + 128]
}

proc handle_ping { } {
#
# Waiting for RQN sync
#
	global SIO MyRQN

	set rqn [get1]

	if { $rqn == $MyRQN } {
		advreq
		return
	}
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
		if { $SIO(VER) > 2 } {
			log "received chunk $rq ([string length $SIO(BUF)])"
		}
	} else {
		if { $SIO(VER) > 1 } {
			log "received superfluous chunk $rq"
		}
	}
}

proc user_dump { par } {
#
# Dump EEPROM
#
	global YourLink SIO EEPROMSIZE ICHUNKSIZE CHUNKS MyLink MyRQN CCOD

	if { $YourLink == 0 } {
		log "don't know about the node yet"
		return
	}

	# two numbers: from, length

	set fwa [exnum par]
	set len [exnum par]

	# file name
	set fil [string trim $par]

	if { $fwa == "" || $len == "" } {
		log "illegal EEPROM parameters"
		return
	}

	set lim [expr $fwa + $len]

	if { $fwa < 0 || $fwa >= $EEPROMSIZE || $len <= 0 ||
	    $lim < $fwa || $lim > $EEPROMSIZE } {
		log "EEPROM range out of bounds"
		return
	}

	# calculate the number of chunks
	set nc [expr ($len + $ICHUNKSIZE - 1) / $ICHUNKSIZE]

	# initialize the chunk table
	for { set ic 0 } { $ic < $nc } { incr ic } {
		set CHUNKS($ic) ""
	}

	# total chunks to be received
	set SIO(NCK) $nc

	# remaining chunks
	set SIO(RCK) $nc

	log "wait ..."

	# send the request packet
	inirqm SEND

	put4 $fwa
	put4 $len

	msg_close

	# keep sending the message until you receive an empty chunk
	set_handler CHUNK handle_chunk

	retrans 512 8

	if $SIO(ADV) {
		log "initial handshake failed, code $SIO(ADV)"
		clear_handlers
		array unset CHUNKS
		return
	}

	# start rounds

	while { [go_prompt] } {

		retrans 512 8

		if $SIO(ADV) {
			log "request interrupted, code $SIO(ADV)"
			clear_handlers
			array unset CHUNKS
			return
		}
	}

	# all chunks received, send the stop packet
	init_msg $MyLink
	put1 $CCOD(GO)
	put1 $MyRQN
	msg_close
	w_serial
	after 128
	w_serial
	after 128
	w_serial

	clear_handlers

	unset SIO(MSG)

	# combine all the chunks into a single block of data
	set chk ""
	for { set ic 0 } { $ic < $nc } { incr ic } {
		append chk $CHUNKS($ic)
	}

	array unset CHUNKS

	set chk [string range $chk 0 [expr $len - 1]]

	if { $fil == "" } {
		# show
		set lng $len
		set org $fwa

		while { $lng } {
			# start new line
			puts -nonewline [format "%08x:" $org]
			for { set ix 0 } { $ix < 16 } { incr ix } {
				if { $lng == 0 } {
					break
				}
				set cc [string index $chk $ix]
				if { $cc == "" } { set cc " " }
				binary scan $cc c code
				puts -nonewline [format " %02x" \
							[expr $code & 0xff]]
				incr lng -1
			}
			puts ""
			if { $lng > 0 } {
				set chk [string range $chk 16 end]
				incr org 16
			}
		}
		return
	}

	# write it to the file

	if [catch { open $fil "w" } fd] {
		log "cannot write to file $fil: $fd"
		return
	}

	fconfigure $fd -translation binary

	if [catch { puts -nonewline $fd $chk } er] {
		log "write error, file $fil: $er"
	}
	catch { close $fd }
	log "transaction complete"
}

proc handle_go { } {
#
# Unpack the requested list of chunks
#
	global SIO REQCK MyRQN YourLink

	if { $SIO(LID) != $YourLink } {
		return
	}

	set rq [get1]
	if { $rq != $MyRQN } {
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

proc user_load { par } {
#
# load EEPROM
#
	global SIO YourLink MyRQN EEPROMSIZE CHUNKS ICHUNKSIZE CHAR INTV REQCK
	global CCOD LPR

	if { $YourLink == 0 } {
		log "don't know about the node yet"
		return
	}

	# starting address
	set fwa [exnum par]
	if { $fwa == "" } {
		log "illegal numerical parameters (FWA)"
		return
	}

	if { $fwa < 0 || $fwa >= $EEPROMSIZE } {
		log "FWA out of range"
		return
	}

	set SIO(MSG) ""
	set val [exnum par]

	if { $val == "" } {
		# try a filename
		set fil [string trim $par]
		if { $fil == "" } {
			log "filename missing"
			return
		}
		# open it and read in
		if [catch { open $fil "r" } fd] {
			log "cannot open $fil: $fd"
			return
		}
		fconfigure $fd -translation binary
		if [catch { read $fd } SIO(MSG)] {
			log "cannot read $fil: $SIO(MSG)"
			unset SIO(MSG)
			catch { close $fd }
			return
		}
		# don't need the file any more
		catch { close $fd }
	} else {
		# input string
		put1 $val
		while 1 {
			set val [exnum par]
			if { $val == "" } {
				break
			}
			put1 $val
		}
	}

	set lng [string length $SIO(MSG)]

	if { $lng == 0 } {
		log "zero bytes to write"
		unset SIO(MSG)
		return
	}

	set lim [expr $fwa + $lng]
	if { $lim < $fwa || $lim > $EEPROMSIZE } {
		log "sequence to write extends beyond EEPROM's end"
		unset SIO(MSG)
		return
	}

	# prepare the chunks
	set cn 0

	while 1 {
		set cs [expr $cn * $ICHUNKSIZE]
		if { $cs >= $lng } {
			break
		}
		set ce [expr $cs + $ICHUNKSIZE - 1]
		set CHUNKS($cn) [string range $SIO(MSG) $cs $ce]
		while { $ce >= $lng } {
			# for the last chunk
			append CHUNKS($cn) $CHAR(0)
			incr ce -1
		}
		incr cn
	}

	unset SIO(MSG)

	log "$lng bytes to write ($cn chunks)"

	log "wait ..."

	# prepare the request message
	inirqm RECV
	put4 $fwa
	put4 $lng

	msg_close

	set_handler GO handle_go

	retrans 512 8

	if $SIO(ADV) {
		log "initial reception failure, code $SIO(ADV)"
		clear_handlers
		array unset CHUNKS
		return
	}

	while 1 {

		clear_handlers
		# received a GO
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
				if { [expr rand ()] > $LPR } {
					init_msg $YourLink
					put1 $CCOD(CHUNK)
					put1 $MyRQN
					put2 $fp
					if [info exists CHUNKS($fp)] {
						append SIO(MSG) $CHUNKS($fp)
					} else {
						# just in case
						append SIO(MSG) $CHUNKS(0)
					}
					msg_close
					w_serial
				} else {

					if { $SIO(VER) > 0 } {
						log "lost chunk: $fp"
					}
				}
				after $INTV(SPACE)
				incr fp
			}
		}

		set_handler GO handle_go

		# send an empty chunk
		init_msg $YourLink
		put1 $CCOD(CHUNK)
		put1 $MyRQN
		msg_close
		
		retrans 512 8

		if $SIO(ADV) {
			if { $SIO(VER) > 0 } {
				log "GO timeout, code $SIO(ADV)"
			}
			break
		}
	}

	clear_handlers
	array unset CHUNKS
	log "transaction complete"
}

proc user_erase { par } {
#
# Erase flash
#
	global YourLink SIO EEPROMSIZE

	if { $YourLink == 0 } {
		log "don't know about the node yet"
		return
	}

	set fwa [exnum par]
	set len [exnum par]

	if { $fwa == "" } {
		# assume all
		set fwa 0
		set len 0
	} else {
		if { $len == "" } {
			log "illegal EEPROM parameters"
			return
		}
		set lim [expr $fwa + $len]
		if { $fwa < 0 || $fwa >= $EEPROMSIZE || $len <= 0 ||
		    $lim < $fwa || $lim > $EEPROMSIZE } {
			log "EEPROM range out of bounds"
			return
		}
	}

	log "wait ..."
	
	inirqm ERAS

	put4 $fwa
	put4 $len

	msg_close

	set_handler PING handle_ping

	retrans 4096 4

	if $SIO(ADV) {
		log "response timeout"
	} else {
		log "done"
	}

	clear_handlers
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

	global Debug DFile

	if $Debug {
		if ![info exists DFile] {
			set DFile [open "Debug" w]
		}
		puts $DFile $m
		flush $DFile
	}
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
