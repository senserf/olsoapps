#!/bin/sh
###########################\
exec tclsh "$0" "$@"

proc msource { f } {
#
# Intelligent 'source'
#
	if ![catch { uplevel #0 source $f } ] {
		# found it right here
		return
	}

	set dir "Scripts"
	set lst ""

	for { set i 0 } { $i < 10 } { incr i } {
		set dir "../$dir"
		set dno [file normalize $dir]
		if { $dno == $lst } {
			# no progress
			break
		}
		if ![catch { uplevel #0 source [file join $dir $f] } ] {
			# found it
			return
		}
	}

	# failed
	puts stderr "Cannot locate file $f 'sourced' by the script."
	exit 99
}

msource oss_u.tcl
msource oss_u_ab.tcl
msource oep.tcl

package require oep 1.0

set CMDS	{ eeget eeput imget imput ctest }

proc log { msg } {

	puts $msg
}

proc uget { msg } {
#
# Receive a normal command response line from the board
#
	puts $msg
}

proc sget { } {
#
# STDIN becomes readable
#
	global ST

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

	if { $line == "!!" } {
		# previous command
		if { $ST(PCM) == "" } {
			log "no previous rejected command"
			return
		}
		set line $ST(PCM)
		set ST(PCM) ""
	}

	if ![u_ab_ready] {
		log "board busy"
		set ST(PCM) $line
		return
	}

	if { [string index $line 0] == "!" } {
		# not for the board
		if [icmd [string trimleft \
		    [string range $line 1 end]]] {
			# failed
			set ST(PCM) $line
		}
		return
	}

	# send it to the board
	u_ab_write $line
}

proc icmd { cmd } {
#
# A command addressed to us
#
	global CMDS

	if ![regexp "^(\[a-z\]+)(.*)" $cmd junk cmd args] {
		log "illegal command syntax"
		return 1
	}

	foreach c $CMDS {
		set kwd [lindex $c 0]
		if { [string first $cmd $c] == 0 } {
			# found
			return [$kwd [string trimleft $args]]
		}
	}

	log "no such command: $cmd"
	return 1
	
}

proc exnum { s } {
#
# Extracts a number from the argument string
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

proc gfile { fil } {
#
# Read the (binary) contents of the specified file
#
	if [catch { open $fil "r" } fd] {
		log "cannot open $fil: $fd"
		return ""
	}

	fconfigure $fd -translation binary -eofchar ""

	if [catch { read $fd } res] {
		log "cannot read $fil: $res"
		catch { close $fd }
		return ""
	}

	catch { close $fd }

	if { $res == "" } {
		log "the file is empty"
	}

	return $res
}

proc sfile { fil stuff } {
#
# Write stuff to the binary file
#
	if [catch { open $fil "w" } fd] {
		log "cannot open $fil for writing: $fd"
		return 1
	}

	fconfigure $fd -translation binary -eofchar ""

	if [catch { puts -nonewline $fd $stuff } res] {
		log "cannot wrtite to $fil: $res"
		catch { close $fd }
		return 1
	}

	catch { close $fd }

	return 0
}

proc updrqn { } {

	global ST

	incr ST(RQN)

	if { $ST(RQN) == 256 } {
		set ST(RQN) 1
	}
}

proc eeput_chunked { chunks } {
#
# Write to EEPROM (chunked version)
#
	global PM ST TM

	set NC 0
	while { $chunks != "" } {
		if { [binary scan $chunks iii mag fwa len] != 3 } {
			log "illegal file format"
			break
		}
		set mag [format %08x $mag]
		if { $mag != "caca1112" } {
			log "illegal magic code $mag"
			break
		}
		if { $fwa < 0 || $fwa >= $PM(EESIZE) } {
			log "origin our of range: $fwa"
			break
		}
		set lim [expr $fwa + $len]
		if { $lim < $fwa || $lim > $PM(EESIZE) } {
			log "chunks extends beyond EEPROM's length: $fwa, $len"
			break
		}
		set chunk [string range $chunks 12 [expr $len + 11]]

		if { [string length $chunk] != $len } {
			log "file ends prematurely"
			break
		}

		log "Sending chunk [format %03d $NC]: [format %08x $fwa]\
			\[[format %08x $len]\]"

		# everything's kosher so far; remove the chunk from the file
		set chunks [string range $chunks [expr $len + 12] end]

		# one OEP request per chunk
		updrqn

		# issue a request to the board
		u_ab_write "re $PM(LID) $ST(RQN) $fwa $len"
		if [u_ab_wait $TM(OUT)] {
			log "request timeout"
			break
		}

		# deactivate AB
		u_ab_setif
		# invoke OEP
		set st [oep_snd $PM(LID) $ST(RQN) $chunk]
		# reactivate AB
		u_ab_setif uget $PM(MPL)

		if $st {
			log "OEP failure $st"
			break
		}
		incr NC
	}

	log "$NC chunks written"

	if { $chunks == "" } {
		return 0
	}

	return 1
}

proc eeput { par } {
#
# Write to EEPROM
#
	global PM ST TM

	# starting address
	set fwa [exnum par]
	if { $fwa == "" } {
		set cnt ""
	} else {
		if { $fwa < 0 || $fwa >= $PM(EESIZE) } {
			log "FWA out of range"
			return 1
		}
		# check fr count
		set cnt [exnum par]
	}

	if { $cnt == "" } {
		# need a filename
		set fil [string trim $par]
		if { $fil == "" } {
			log "filename missing"
			return 1
		}
		set chunks [gfile $fil]
		if { $chunks == "" } {
			# failure
			return 1
		}
	} else {
		# input string
		set chunks [binary format c $cnt]
		while 1 {
			set cnt [exnum par]
			if { $cnt == "" } {
				break
			}
			append chunks [binary format c $cnt]
		}
	}

	if { $fwa == "" } {
		# handle the chunked format
		return [eeput_chunked $chunks]
	}

	# note that this cannot be zero
	set len [string length $chunks]
	set lim [expr $fwa + $len]

	if { $lim < $fwa || $lim > $PM(EESIZE) } {
		log "sequence to write extends beyond EEPROM's end"
		return 1
	}

	updrqn

	# issue a request to the board
	u_ab_write "re $PM(LID) $ST(RQN) $fwa $len"
	if [u_ab_wait $TM(OUT)] {
		log "request timeout"
		return 1
	}

	# deactivate AB
	u_ab_setif
	# invoke OEP
	set st [oep_snd $PM(LID) $ST(RQN) $chunks]
	# reactivate AB
	u_ab_setif uget $PM(MPL)

	if $st {
		log "OEP failure"
	}

	return $st
}

proc eeget { par } {
#
# Get an EEPROM range from the board
#
	global PM TM ST

	set fwa [exnum par]
	set len [exnum par]

	# otional file name
	set fil [string trim $par]

	if { $fwa == "" || $len == "" } {
		log "illegal EEPROM parameters"
		return 1
	}

	set lim [expr $fwa + $len]

	if { $fwa < 0 || $fwa >= $PM(EESIZE) || $len <= 0 ||
	    $lim < $fwa || $lim > $PM(EESIZE) } {
		log "EEPROM range out of bounds"
		return 1
	}

	# calculate the number of chunks
	set nc [expr ($len + $PM(ICSIZE) - 1) / $PM(ICSIZE)]

	# advance request number
	updrqn

	# issue a request to the board
	u_ab_write "se $PM(LID) $ST(RQN) $fwa $len"
	if [u_ab_wait $TM(OUT)] {
		log "request timeout"
		return 1
	}

	# deactivate AB
	u_ab_setif
	# invoke OEP
	set chunks [oep_rcv $PM(LID) $ST(RQN) $nc]
	# re-activate AB
	u_ab_setif uget $PM(MPL)

	if { $chunks == "" } {
		log "OEP timeout"
		return 1
	}

	set chunks [string range $chunks 0 [expr $len - 1]]

	# got it! 
	if { $fil != "" } {
		if [sfile $fil $chunks] {
			return 1
		}
		return 0
	}

	# otherwise, display that stuff

	while { $len } {
		# start new line
		puts -nonewline [format "%08x:" $fwa]
		for { set ix 0 } { $ix < 16 } { incr ix } {
			if { $len == 0 } {
				break
			}
			set cc [string index $chunks $ix]
			if { $cc == "" } { set cc " " }
			binary scan $cc c code
			puts -nonewline [format " %02x" [expr $code & 0xff]]
			incr len -1
		}
		puts ""
		if { $len > 0 } {
			set chunks [string range $chunks 16 end]
			incr fwa 16
		}
	}
	return 0
}

proc ilget { msg } {
#
# For receiving the list of images
#
	global ST TM

	if [regexp "^Err.*(\[0-9\]+)" $msg junk code] {
		# done
		catch { after cancel $ST(CBA) }
		set ST(DON) $code
		return
	}

	if [regexp "^OK" $msg] {
		catch { after cancel $ST(CBA) }
		set ST(DON) 0
		return
	}

	if ![regexp "(\[0-9\]+)\[^0-9\]+(\[0-9\]+)\[^0-9\]+(\[0-9\]+).*:(.*)" \
	    $msg junk han x y lab] {
		return
	}

	catch { after cancel $ST(CBA) }

	lappend ST(ILI) [list $han $x $y [string trim $lab]]

	set ST(CBA) [after $TM(OUT) stmout]
}

proc stmout { } {

	global ST

	set ST(DON) 999
}

proc rilist { } {
#
# (Re)read image list
#
	global ST TM PM

	log "retrieving image list from board ..."

	# intercept input from the board
	u_ab_setif ilget $PM(MPL)

	set ST(ILI) ""
	set ST(DON) -1

	u_ab_write "li"
	if [u_ab_wait $TM(OUT)] {
		log "request timeout"
		return 1
	}

	if { $ST(DON) < 0 } {
		# ilget may finish earlier
		set ST(CBA) [after $TM(OUT) stmout]
		vwait ST(DON)
	}

	u_ab_setif uget $PM(MPL)

	if $ST(DON) {
		log "error, status = $ST(DON)"
		return 1
	}

	if { $ST(ILI) == "" } {
		log "done, no images available"
		return 1
	} 

	puts "done, [llength $ST(ILI)] images available"
	return 0
}

proc imput { par } {
#
# Send an image to the board
#
	global ST PM TM

	# file name
	set fil [string trim $par]
	if { $fil == "" } {
		log "file name required"
		return 1
	}

	set chunks [gfile $fil]

	if { $chunks == "" } {
		return 1
	}

	set len [string length $chunks]

	set hdl [expr $PM(LBL) + 4]
	if { $len < $hdl } {
		log "image file formar error"
		return 1
	}

	binary scan $chunks scc mag x y

	set mag [expr $mag & 0xffff]
	set x [expr $x & 0xff]
	set y [expr $y & 0xff]


	if { $mag != $PM(IMA) || $x < 4 || $x > $PM(MXX) || $y < 4 ||
	    $y > $PM(MYY) } {
		log "image file format error"
		return 1
	}

	# assume the label is ASCII
	set lbl ""
	for { set nc 4 } { $nc < [expr $PM(LBL) + 4] } { incr nc } {
		set cl [string index $chunks $nc]
		if { $cl == "\\x00" } {
			break
		}
		append lbl $cl
	}

	# calculate the number of chunks
	set nc [expr ($x * $y + $PM(PPC) - 1)/$PM(PPC)]

	# extract the proper label
	set chunks [string range $chunks [expr $PM(LBL) + 4] end]
	# the length of the chunk set
	set len [expr $len - $PM(LBL) - 4]

	# file length rounded up to full chunks
	set cl [expr $nc * $PM(ICSIZE)]

	if { $len > $cl || [expr $cl - $len] >= $PM(ICSIZE) } {
		log "image file length is incorrect"
		return 1
	}

	while { $len < $cl } {
		append chunks \x00
		incr len
	}

	log "switching to OEP"

	updrqn

	# issue a request to the board
	u_ab_write "ri $PM(LID) $ST(RQN) $x $y $lbl"
	if [u_ab_wait $TM(OUT)] {
		log "request timeout"
		return 1
	}

	# deactivate AB
	u_ab_setif
	# invoke OEP
	set st [oep_snd $PM(LID) $ST(RQN) $chunks]
	# reactivate AB
	u_ab_setif uget $PM(MPL)

	if $st {
		log "OEP failure"
	}

	return $st
}

proc imget { par } {
#
# Retrieve an image
#
	global ST PM TM

	set han [exnum par]

	if { $han == "" } {
		log "no handle specified"
		return 1
	}

	# file name
	set fil [string trim $par]
	if { $fil == "" } {
		log "file name required"
		return 1
	}

	if [rilist] {
		return 1
	}

	set nf 1
	foreach im $ST(ILI) {
		if { [lindex $im 0] == $han } {
			set nf 0
			break
		}
	}

	if $nf {
		log "no such handle"
		return 1
	}
	set x [lindex $im 1]
	set y [lindex $im 2]

	# calculate the number of chunks
	set nc [expr ($x * $y + $PM(PPC) - 1)/$PM(PPC)]

	log "switching to OEP"

	# advance request number
	updrqn

	# issue a request to the board
	u_ab_write "si $PM(LID) $ST(RQN) $han"
	if [u_ab_wait $TM(OUT)] {
		log "request timeout"
		return 1
	}

	# deactivate AB
	u_ab_setif
	# invoke OEP
	set chunks [oep_rcv $PM(LID) $ST(RQN) $nc]
	# re-activate AB
	u_ab_setif uget $PM(MPL)

	if { $chunks == "" } {
		log "OEP timeout"
		return 1
	}

	# build the header
	set hdr [binary format scc $PM(IMA) $x $y]
	# append the label
	set lab [string range [lindex $im 3] 0 [expr $PM(LBL) - 1]]
	set len [string length $lab]
	while { $len < $PM(LBL) } {
		append lab \x00
		incr len
	}
	# this should be fixed length = 38 bytes
	append hdr $lab

	# store the thing
	if [sfile $fil "$hdr$chunks"] {
		return 1
	}
	return 0
}

proc issue { cmd } {
#
# Issue a simple command to the board
#
	global TM PM

	u_ab_write $cmd
	if [u_ab_wait $TM(LNG)] {
		log "reply timeout '$cmd'"
		u_ab_setif uget $PM(MPL)
		return 1
	}

	return 0
}

proc empty { par } { }

proc ctest { par } {
#
# Color test
#
	global PM

	set cf [exnum par]

	if { $cf == "" } {
		log "bg color required"
		return 1
	}

	set cb [exnum par]

	u_ab_setif empty $PM(MPL)
	# set the colors
	if [issue "os 10 [format %04x $cf]"] {
		return 1
	}

	if { $cb != "" } {
		if [issue "os 9 [format %04x $cb]"] {
			return 1
		}
	}

	if [issue "dd 31"] {
		return 1
	}

	if [issue "ct 31 0 10 9 10 60 10"] {
		return 1
	}

	if [issue "THIS IS A TEST LINE"] {
		return 1
	}

	if [issue "da 31"] {
		return 1
	}

	u_ab_setif uget $PM(MPL)
	log "Done"

	return 0
}

######### COM port ############################################################

set PM(EESIZE)		[expr 256 * 2048]
set PM(ICSIZE)		54
set PM(TCSIZE)		56
# this is the link ID we are going to use
set PM(LID)		3
# pixels per chunk
set PM(PPC)		[expr (54*8)/12]
# image magic
set PM(IMA)		0x77AC
# max total label length
set PM(LBL)		38
# max Y and Y for an image
set PM(MXX)		130
set PM(MYY)		130

# maximum packet length (use OEP_MAXRAWPL, which will limit the line size
# for UART to that of CC1100 - framing
set PM(MPL)		60

# connected to board's command interface
set ST(RQN)	1

# image list
set ST(ILI)	""

# request acceptance timeout
set TM(OUT)	2048
set TM(LNG)	5000

if [catch { u_start "" 115200 "" } err] {
	log $err
	exit 99
}

u_ab_setif uget $PM(MPL)

fconfigure stdin -buffering line -blocking 0
fileevent stdin readable sget

#u_settrace 7 dump.txt

vwait None
