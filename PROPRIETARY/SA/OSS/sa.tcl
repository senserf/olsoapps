#!/bin/sh
#####################\
exec wish85 "$0" "$@"

###############################################################################
# Determine the system type ###################################################
###############################################################################
if [catch { exec uname } ST(SYS)] {
	set ST(SYS) "W"
} elseif [regexp -nocase "linux" $ST(SYS)] {
	set ST(SYS) "L"
} elseif [regexp -nocase "cygwin" $ST(SYS)] {
	set ST(SYS) "C"
} else {
	set ST(SYS) "W"
}
if { $ST(SYS) != "L" } {
	set u [string trimright [lindex $argv end]]
	if { $u == "" } {
		set argv [lreplace $argv end end]
	} else {
		set argv [lreplace $argv end end $u]
	}
	unset u
}

###############################################################################
# Determine the way devices are named; if running natively under Cygwin, use
# Linux style
###############################################################################

if [file isdirectory "/dev"] {
	set ST(DEV) "L"
} else {
	set ST(DEV) "W"
}

###############################################################################

set ST(VER) 0.84

## double exit avoidance flag
set DEAF 0

###############################################################################

package provide uartpoll 1.0
#################################################################
# Selects polled versus automatic input from asynchronous UART. #
# Copyright (C) 2012 Olsonet Communications Corporation.        #
#################################################################

namespace eval UARTPOLL {

variable DE

proc run_picospath { } {

	set ef [auto_execok "picospath"]
	if ![file executable $ef] {
		return [eval [list exec] [list sh] [list $ef] [list -d]]
	}
	return [eval [list exec] [list $ef] [list -d]]
}

proc get_deploy_param { } {
#
# Extracts the poll argument of deploy
#
	if [catch { run_picospath } a] {
		return ""
	}

	# look up -p
	set l [lsearch -exact $a "-p"]
	if { $l < 0 } {
		return ""
	}

	set l [lindex $a [expr $l + 1]]

	if [catch { expr $l } l] {
		return 40
	}

	if { $l < 0 } {
		return 0
	}

	return [expr int($l)]
}

proc uartpoll_interval { sys fsy } {

	set p [get_deploy_param]

	if { $p != "" } {
		return $p
	}

	# use heuristics
	if { $sys == "L" } {
		# We are on Linux, the only problem is virtual box
		if { [file exists "/dev/vboxuser"] ||
		     [file exists "/dev/vboxguest"] } {
			# VirtualBox; use a longer timeout
			return 400
		}
		return 0
	}

	if { $fsy == "L" } {
		# Cygwin + native Tcl
		return 40
	}

	return 0
}

proc read_cb { dev fun } {

	variable CB

	if [eval $fun] {
		# void call, push the timeout
		if { $CB($dev) < $CB($dev,m) } {
			incr CB($dev)
		}
	} else {
		set $CB($dev) 0
	}

	set CB($dev,c) [after $CB($dev) "::UARTPOLL::read_cb $dev $fun"]
}

proc uartpoll_oninput { dev fun { sys "" } { fsy "" } } {

	variable CB

	if { $sys == "" } {
		set p 0
	} else {
		set p [uartpoll_interval $sys $fsy]
	}

	if { $p == 0 } {
		set CB($dev,c) ""
		fileevent $dev readable $fun
		return
	}

	set CB($dev) 1
	set CB($dev,m) $p
	read_cb $dev $fun
}

proc uartpoll_stop { dev } {

	variable CB

	if [info exists CB($dev)] {
		if { $CB($dev,c) != "" } {
			catch { after cancel $CB($dev,c) }
		}
		array unset CB($dev)
		array unset CB($dev,*)
	}
}

namespace export uartpoll_*

### end of UARTPOLL namespace #################################################

}

namespace import ::UARTPOLL::uartpoll_*

package provide unames 1.0
##########################################################################
# This is a package for handling the various names under which COM ports #
# may appear in our messy setup.                                         #
# Copyright (C) 2012 Olsonet Communications Corporation.                 #
##########################################################################

namespace eval UNAMES {

variable Dev

proc unames_init { dtype { stype "" } } {

	variable Dev

	# device layout type: "L" (Linux), other "Windows"
	set Dev(DEV) $dtype
	# system type: "L" (Linux), other "Windows/Cygwin"
	set Dev(SYS) $stype

	if { $Dev(DEV) == "L" } {
		# determine the root of virtual ttys
		if [file isdirectory "/dev/pts"] {
			# BSD style
			set Dev(PRT) "/dev/pts/%"
		} else {
			set Dev(PRT) "/dev/pty%"
		}
		# number bounds (inclusive) for virtual devices
		set Dev(PRB) { 0 8 }
		# real ttys
		if { $Dev(SYS) == "L" } {
			# actual Linux
			set Dev(RRT) "/dev/ttyUSB%"
		} else {
			# Cygwin
			set Dev(RRT) "/dev/ttyS%"
		}
		# and their bounds
		set Dev(RRB) { 0 31 }
	} else {
		set Dev(PRT) { "CNCA%" "CNCB%" }
		set Dev(PRB) { 0 3 }
		set Dev(RRT) "COM%:"
		set Dev(RRB) { 1 32 }
	}

	unames_defnames
}

proc unames_defnames { } {
#
# Generate the list of default names
#
	variable Dev

	# flag == default names, not real devices
	set Dev(DEF) 1
	# true devices
	set Dev(COM) ""
	# virtual devices
	set Dev(VCM) ""

	set rf [lindex $Dev(RRB) 0]
	set rt [lindex $Dev(RRB) 1]
	set pf [lindex $Dev(PRB) 0]
	set pt [lindex $Dev(PRB) 1]

	while { $rf <= $rt } {
		foreach d $Dev(RRT) {
			regsub "%" $d $rf d
			lappend Dev(COM) $d
		}
		incr rf
	}

	while { $pf <= $pt } {
		foreach d $Dev(PRT) {
			regsub "%" $d $pf d
			lappend Dev(VCM) $d
		}
		incr pf
	}
}

proc unames_ntodev { n } {
#
# Proposes a device list for a number
#
	variable Dev

	regsub -all "%" $Dev(RRT) $n d
	return $d
}

proc unames_ntovdev { n } {
#
# Proposes a virtual device list for a number
#
	variable Dev

	regsub -all "%" $Dev(PRT) $n d

	if { $Dev(DEV) == "L" && ![file exists $d] } {
		# this is supposed to be authoritative
		return ""
	}
	return $d
}

proc unames_unesc { dn } {
#
# Escapes the device name so it can be used as an argument to open
#
	variable Dev

	if { $Dev(DEV) == "L" } {
		# no need to do anything
		return $dn
	}

	if [regexp -nocase "^com(\[0-9\]+):$" $dn jk pn] {
		set dn "\\\\.\\COM$pn"
	} else {
		set dn "\\\\.\\$dn"
	}

	return $dn
}

proc unames_scan { } {
#
# Scan actual devices
#
	variable Dev

	set Dev(DEF) 0
	set Dev(COM) ""
	set Dev(VCM) ""

	# real devices
	for { set i 0 } { $i < 256 } { incr i } {
		set dl [unames_ntodev $i]
		foreach d $dl {
			if [catch { open [unames_unesc $d] "r" } fd] {
				continue
			}
			catch { close $fd }
			lappend Dev(COM) $d
		}
	}

	for { set i 0 } { $i < 32 } { incr i } {
		set dl [unames_ntovdev $i]
		if { $dl == "" } {
			continue
		}
		if { $Dev(DEV) == "L" } {
			# don't try to open them; unames_ntovdev is
			# authoritative, and opening those representing
			# terminals may mess them up
			foreach d $dl {
				lappend Dev(VCM) $d
			}
			continue
		}
		foreach d $dl {
			if [catch { open [unames_unesc $d] "r" } fd] {
				continue
			}
			catch { close $fd }
			lappend Dev(VCM) $d
		}
	}
}

proc unames_fnlist { fn } {
#
# Returns the list of filenames to try to open, given an element from one of
# the lists; if not on the list, assume a direct name (to be escaped, however)
#
	variable Dev

	if [regexp "^\[0-9\]+$" $fn] {
		# just a number
		return [unames_ntodev $fn]
	}

	if { [lsearch -exact $Dev(COM) $fn] >= 0 } {
		if !$Dev(DEF) {
			# this is an actual device
			return $fn
		}
		# get a number and convert to a list
		if ![regexp "\[0-9\]+" $fn n] {
			return ""
		}
		return [unames_ntodev $n]
	}
	if { [lsearch -exact $Dev(VCM) $fn] >= 0 } {
		if !$Dev(DEF) {
			return $fn
		}
		if ![regexp "\[0-9\]+" $fn n] {
			return ""
		}
		return [unames_ntovdev $n]
	}
	# return as is
	return $fn
}

proc unames_choice { } {

	variable Dev

	return [list $Dev(COM) $Dev(VCM)]
}

namespace export unames_*

### end of UNAMES namespace ###################################################
}

namespace import ::UNAMES::*

package provide tooltips 1.0

###############################################################################
# TOOLTIPS ####################################################################
###############################################################################

namespace eval TOOLTIPS {

variable ttps

proc tip_init { { fo "" } { wi "" } { bg "" } { fg "" } } {
#
# Font, wrap length, i.e., width (in pixels)
#
	variable ttps

	if { $fo == "" } {
		set fo "TkSmallCaptionFont"
	}

	if { $wi == "" } {
		set wi 320
	}

	if { $bg == "" } {
		set bg "lightyellow"
	}

	if { $fg == "" } {
		set fg "black"
	}

	set ttps(FO) $fo
	set ttps(WI) $wi
	set ttps(BG) $bg
	set ttps(FG) $fg

}

proc tip_set { w t } {

	bind $w <Any-Enter> [list after 200 [list tip_show %W $t]]
	bind $w <Any-Leave> [list after 500 [list destroy %W.ttip]]
	bind $w <Any-KeyPress> [list after 500 [list destroy %W.ttip]]
	bind $w <Any-Button> [list after 500 [list destroy %W.ttip]]
}

proc tip_show { w t } {

	global tcl_platform
	variable ttps

	set px [winfo pointerx .]
	set py [winfo pointery .]

	if { [string match $w* [winfo containing $px $py]] == 0 } {
                return
        }

	catch { destroy $w.ttip }

	set scrh [winfo screenheight $w]
	set scrw [winfo screenwidth $w]

	set tip [toplevel $w.ttip -bd 1 -bg black]

	wm geometry $tip +$scrh+$scrw
	wm overrideredirect $tip 1

	if { $tcl_platform(platform) == "windows" } {
		wm attributes $tip -topmost 1
	}

	pack [label $tip.label -bg $ttps(BG) -fg $ttps(FG) -text $t \
		-justify left -wraplength $ttps(WI) -font $ttps(FO)]

	set wi [winfo reqwidth $tip.label]
	set hi [winfo reqheight $tip.label]

	set xx [expr $px - round($wi / 2.0)]

	if { $py > [expr $scrh / 2.0] } {
		# lower half
		set yy [expr $py - $hi - 10]
	} else {
		set yy [expr $py + 10]
	}

	if  { [expr $xx + $wi] > $scrw } {
		set xx [expr $scrw - $wi]
	} elseif { $xx < 0 } {
		set xx 0
	}

	wm geometry $tip [join "$wi x $hi + $xx + $yy" {}]

        raise $tip

        bind $w.ttip <Any-Enter> { destroy %W }
        bind $w.ttip <Any-Leave> { destroy %W }
}

namespace export tip_*

}

namespace import ::TOOLTIPS::*

###############################################################################
# End of TOOLTIPS #############################################################
###############################################################################

package require tooltips

###############################################################################

package provide noss 1.0

###############################################################################
# NOSS ########################################################################
###############################################################################

# This is N-mode packet interface with the Network ID field used as part
# of payload (akin to the boss package)

namespace eval NOSS {

###############################################################################
# ISO 3309 CRC + supplementary stuff needed by the protocol module ############
###############################################################################

variable CRCTAB {
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

variable B

# abort function (in case of internal fatal error, considered impossible)
set B(ABR) ""

# diag output function
set B(DGO) ""

# character zero (aka NULL)
set B(ZER) [format %c [expr 0x00]]

# preamble byte
set B(IPR) [format %c [expr 0x55]]

# diag preamble (for packet modes) = ASCII DLE
set B(DPR) [format %c [expr 0x10]]

# reception automaton state
set B(STA) 0

# reception automaton remaining byte count
set B(CNT) 0

# function to call on packet reception
set B(DFN) ""

# function to call on UART close (which can happen asynchronously)
set B(UCF) ""

# low-level reception timer
set B(TIM)  ""

# packet timeout (msec), once reception has started
set B(PKT) 8000

###############################################################################

proc noss_chks { wa } {

	variable CRCTAB

	set nb [string length $wa]

	set chs 0

	while { $nb > 0 } {

		binary scan $wa su waw
		#set waw [expr $waw & 0x0000ffff]

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

proc no_abort { msg } {

	variable B

	if { $B(ABR) != "" } {
		$B(ABR) $msg
		exit 1
	}

	catch { puts stderr $msg }
	exit 1
}

proc no_diag { } {

	variable B

	if { [string index $B(BUF) 0] == $B(ZER) } {
		# binary
		set ln [string range $B(BUF) 3 5]
		binary scan $ln cuSu lv code
		set ln "\[[format %02x $lv] -> [format %04x $code]\]"
	} else {
		# ASCII
		set ln "[string trim $B(BUF)]"
	}

	if { $B(DGO) != "" } {
		$B(DGO) $ln
	} else {
		puts "DIAG: $ln"
		flush stdout
	}
}

proc gize { fun } {

	if { $fun != "" && [string range $fun 0 1] != "::" } {
		set fun "::$fun"
	}

	return $fun
}

proc lize { fun } {

	return "::NOSS::$fun"
}

proc no_emu_readable { fun } {
#
# Emulates auto read on readable UART
#
	variable B

	if [$fun] {
		# a void call, increase the timeout
		if { $B(ROT) < $B(RMX) } {
			incr B(ROT)
		}
	} else {
		set B(ROT) 0
	}

	set B(ONR) [after $B(ROT) "[lize no_emu_readable] $fun"]
}

proc no_write { msg } {
#
# Writes a packet to the UART
#
	variable B

	set ln [string length $msg]
	if { $ln > $B(MPL) } {
		# truncate the message to size, probably a bad idea
		set ln $B(MPL)
		set msg [string range $msg 0 [expr $ln - 1]]
	}

	if [expr $ln & 1] {
		# need a filler zero byte
		append msg $B(ZER)
		incr ln -1
	} else {
		incr ln -2
	}

	if [catch {
		puts -nonewline $B(SFD) \
			"$B(IPR)[binary format c $ln]$msg[binary format s\
				[noss_chks $msg]]"
		flush $B(SFD)
	} err] {
		noss_close "NOSS write error, $err"
	}
}

proc no_timeout { } {

	variable B

	if { $B(TIM) != "" } {
		no_rawread 1
		set B(TIM) ""
	}
}

proc no_rawread { { tm 0 } } {
#
# Called whenever data is available on the UART; returns 1 (if void), 0 (if
# progress, i.e., some data was available)
#
	variable B
#
#  STA = 0  -> Waiting for preamble
#        1  -> Waiting for the length byte
#        2  -> Waiting for (CNT) bytes until the end of packet
#        3  -> Waiting for end of DIAG preamble
#        4  -> Waiting for EOL until end of DIAG
#        5  -> Waiting for (CNT) bytes until the end of binary diag
#
	set chunk ""
	set void 1

	while 1 {

		if { $chunk == "" } {

			if [catch { read $B(SFD) } chunk] {
				# nonblocking read, ignore errors
				set chunk ""
			}

			if { $chunk == "" } {
				# check for timeout
				if $tm {
					# reset
					set B(STA) 0
				} elseif { $B(STA) != 0 } {
					# something has started, set up timer,
					# if not running already
					if { $B(TIM) == "" } {
						set B(TIM) \
				            	    [after $B(PKT) \
							[lize no_timeout]]
					}
				}
				return $void
			}
			# there is something to process, cancel timeout
			if { $B(TIM) != "" } {
				catch { after cancel $B(TIM) }
				set B(TIM) ""
			}
			set void 0
		}

		set bl [string length $chunk]

		switch $B(STA) {

		0 {
			# Look up the preamble byte in the received string
			for { set i 0 } { $i < $bl } { incr i } {
				set c [string index $chunk $i]
				if { $c == $B(IPR) } {
					# preamble found
					set B(STA) 1
					break
				}
				if { $c == $B(DPR) } {
					# diag preamble
					set B(STA) 3
					break
				}
			}
			if { $i == $bl } {
				# not found, keep waiting
				set chunk ""
				continue
			}
			# found, remove the parsed portion and keep going
			set chunk [string range $chunk [expr $i + 1] end]
		}

		1 {
			# expecting the length byte (note that the byte
			# does not cover the statid field, so its range is
			# up to MPL - 2)
			binary scan [string index $chunk 0] cu bl
			set chunk [string range $chunk 1 end]
			if { [expr $bl & 1] || $bl > [expr $B(MPL) - 2] } {
				# reset
				set B(STA) 0
				continue
			}
			# how many bytes to expect
			set B(CNT) [expr $bl + 4]
			set B(BUF) ""
			# found
			set B(STA) 2
		}

		2 {
			# packet reception, filling the buffer
			if { $bl < $B(CNT) } {
				append B(BUF) $chunk
				set chunk ""
				incr B(CNT) -$bl
				continue
			}

			# end of packet, reset
			set B(STA) 0

			if { $bl == $B(CNT) } {
				append B(BUF) $chunk
				set chunk ""
				# we have a complete buffer
				no_receive
				continue
			}

			# merged packets
			append B(BUF) [string range $chunk 0 [expr $B(CNT) - 1]]
			set chunk [string range $chunk $B(CNT) end]
			no_receive
		}

		3 {
			# waiting for the end of a diag header
			set chunk [string trimleft $chunk $B(DPR)]
			if { $chunk != "" } {
				set B(BUF) ""
				# look at the first byte of diag
				if { [string index $chunk 0] == $B(ZER) } {
					# a binary diag, length == 7
					set B(CNT) 7
					set B(STA) 5
				} else {
					# ASCII -> wait for NL
					set B(STA) 4
				}
			}
		}

		4 {
			# waiting for NL ending a diag
			set c [string first "\n" $chunk]
			if { $c < 0 } {
				append B(BUF) $chunk
				set chunk ""
				continue
			}

			append B(BUF) [string range $chunk 0 $c]
			set chunk [string range $chunk [expr $c + 1] end]
			# reset
			set B(STA) 0
			no_diag
		}

		5 {
			# waiting for CNT bytes of binary diag
			if { $bl < $B(CNT) } {
				append B(BUF) $chunk
				set chunk ""
				incr B(CNT) -$bl
				continue
			}
			# reset
			set B(STA) 0
			append B(BUF) [string range $chunk 0 [expr $B(CNT) - 1]]
			set chunk [string range $chunk $B(CNT) end]
			no_diag
		}

		default {
			set B(STA) 0
		}
		}
	}
}

proc no_receive { } {
#
# Handle a received packet
#
	variable B
	
	# dmp "RCV" $B(BUF)

	# validate CRC
	if [noss_chks $B(BUF)] {
		return
	}

	# strip off the checksum
	set msg [string range $B(BUF) 0 end-2]
	set len [string length $msg]

	if { $len < 2 } {
		# ignore it
		return
	}

	if { $B(DFN) != "" } {
		$B(DFN) $msg
	}
}

proc noss_init { ufd mpl { inp "" } { dia "" } { clo "" } { emu 0 } } {
#
# Initialize: 
#
#	ufd - UART descriptor
#	mpl - max packet length
#	inp - function to be called on user input
#	dia - function to be called to present a diag message
#	clo - function to call on UART close (can happen asynchronously)
#	emu - emulate 'readable'
#
	variable B

	set B(STA) 0
	set B(CNT) 0
	set B(BUF) ""

	set B(SFD) $ufd
	set B(MPL) $mpl

	set B(UCF) $clo

	set B(DFN) $inp
	set B(DGO) $dia

	fconfigure $B(SFD) -buffering full -translation binary

	if $emu {
		# the readable flag doesn't work for UART on some Cygwin
		# setups
		set B(ROT) 1
		set B(RMX) $emu
		no_emu_readable [lize no_rawread]
	} else {
		# do it the easy way
		fileevent $B(SFD) readable [lize no_rawread]
	}
}

proc noss_oninput { { fun "" } } {
#
# Declares a function to be called when a packet is received
#
	variable B

	if { $fun != "" } {
		set fun [gize $fun]
	}

	set B(DFN) $fun
}

proc noss_stop { } {
#
# Stop the protocol
#
	variable B

	if { $B(TIM) != "" } {
		# kill the callback
		catch { after cancel $B(TIM) }
		set B(TIM) ""
	}

	set B(STA) 0
	set B(CNT) 0
	set B(BUF) ""
}

proc noss_close { { err "" } } {
#
# Close the UART (externally or internally, which can happen asynchronously)
#
	variable B

	# trc "NOSS CLOSE"

	if { [info exist B(ONR)] && $B(ONR) != "" } {
		# we have been emulating 'readable', kill the callback
		catch { after cancel $B(ONR) }
		unset B(ONR)
	}

	if { $B(UCF) != "" } {
		# any extra function to call?
		set bucf $B(UCF)
		# prevents recursion loops
		set B(UCF) ""
		$bucf $err
	}

	catch { close $B(SFD) }

	set B(SFD) ""

	# stop the protocol
	noss_stop

	set B(DFN) ""
	set B(DGO) ""
}

proc noss_send { buf } {
#
# This is the user-level output function
#
	variable B

	if { $B(SFD) == "" } {
		# ignore if disconnected, this shouldn't happen
		return
	}

	# dmp "SND" $buf

	no_write $buf
}

namespace export noss_*

}

namespace import ::NOSS::*

package require noss

###############################################################################
###############################################################################

proc trigger { } {

	global __evnt

	set __evnt 1
}

proc event_loop { } {

	global __evnt

	set __evnt 0
	vwait __evnt
}

###############################################################################

package provide autoconnect 1.0

package require unames

###############################################################################
# Autoconnect #################################################################
###############################################################################

namespace eval AUTOCONNECT {

variable ACB

array set ACB { CLO "" CBK "" }

proc gize { fun } {

	if { $fun != "" && [string range $fun 0 1] != "::" } {
		set fun "::$fun"
	}

	return $fun
}

proc lize { fun } {

	return "::AUTOCONNECT::$fun"
}

proc autocn_heartbeat { } {
#
# Called to provide connection heartbeat, typically after a message reception
#
	variable ACB

	incr ACB(LRM)
}

proc autocn_start { op cl hs hc cc { po "" } { dl "" } } {
#
# op - open function
# cl - close function
# hs - command to issue handshake message
# hc - handshake condition (if true, handshake has been successful)
# cc - connection condition (if false, connection has been broken)
# po - poll function (to kick the node if no heartbeat)
# dl - explicit device list
# 
	variable ACB

	set ACB(OPE) [gize $op]
	set ACB(CLO) [gize $cl]
	set ACB(HSH) [gize $hs]
	set ACB(HSC) [gize $hc]
	set ACB(CNC) [gize $cc]
	set ACB(POL) [gize $po]
	set ACB(DLI) $dl

	# trc "ACN START: $ACB(OPE) $ACB(CLO) $ACB(HSH) $ACB(HSC) $ACB(CNC) $ACB(POL) $ACB(DLI)"

	# last device scan time
	set ACB(LDS) 0

	# automaton state
	set ACB(STA) "L"

	# hearbeat variables
	set ACB(RRM) -1
	set ACB(LRM) -1

	if { $ACB(CBK) != "" } {
		# a precaution
		catch { after cancel $ACB(CBK) }
	}
	set ACB(CBK) [after 10 [lize ac_callback]]
}

proc autocn_stop { } {
#
# Stop the callback and disconnect
#
	variable ACB

	if { $ACB(CBK) != "" } {
		catch { after cancel $ACB(CBK) }
		set ACB(CBK) ""
	}

	if { $ACB(CLO) != "" } {
		$ACB(CLO)
	}
}

proc ac_again { d } {

	variable ACB

	set ACB(CBK) [after $d [lize ac_callback]]
}

proc ac_callback { } {
#
# This callback checks if we are connected and if not tries to connect us
# automatically to the node
#
	variable ACB

	# trc "AC S=$ACB(STA)"
	if { $ACB(STA) == "R" } {
		# CONNECTED
		if ![$ACB(CNC)] {
			# just got disconnected
			set ACB(STA) "L"
			ac_again 100
			return
		}
		# we are connected, check for heartbeat
		if { $ACB(RRM) == $ACB(LRM) } {
			# stall?
			if { $ACB(POL) != "" } { $ACB(POL) }
			set ACB(STA) "T"
			set ACB(FAI) 0
			ac_again 1000
			return
		}
		set ACB(RRM) $ACB(LRM)
		# we are still connected, no need to panic
		ac_again 2000
		return
	}

	if { $ACB(STA) == "T" } {
		# HEARTBEAT FAILURE
		if ![$ACB(CNC)] {
			# disconnected
			set ACB(STA) "L"
			ac_again 100
			return
		}
		if { $ACB(RRM) == $ACB(LRM) } {
			# count failures
			if { $ACB(FAI) == 5 } {
				set ACB(STA) "L"
				$ACB(CLO)
				ac_again 100
				return
			}
			incr ACB(FAI)
			if { $ACB(POL) != "" } { $ACB(POL) }
			ac_again 600
			return
		}
		set ACB(RRM) $ACB(LRM)
		set ACB(STA) "R"
		ac_again 2000
		return
	}
	
	#######################################################################

	if { $ACB(STA) == "L" } {
		# CONNECTING
		set tm [clock seconds]
		if { $tm > [expr $ACB(LDS) + 5] } {
			# last scan for new devices was done more than 5 sec
			# ago, rescan
			if { $ACB(DLI) != "" } {
				set ACB(DVS) ""
				foreach d $ACB(DLI) {
					if [catch { expr $d } n] {
						lappend ACB(DVS) $d
					} else {
						lappend ACB(DVS) \
							[unames_ntodev $d]
					}
				}
			} else {
				unames_scan
				set ACB(DVS) [lindex [unames_choice] 0]
			}
			set ACB(DVL) [llength $ACB(DVS)]
			set ACB(LDS) $tm
			# trc "AC RESCAN: $ACB(DVS), $ACB(DVL)"
		}
		# index into the device table
		set ACB(CUR) 0
		set ACB(STA) "N"
		ac_again 250
		return
	}

	#######################################################################

	if { $ACB(STA) == "N" } {
		# TRYING NEXT DEVICE
		# trc "AC N CUR=$ACB(CUR), DVL=$ACB(DVL)"
		# try to open a new UART
		if { $ACB(CUR) >= $ACB(DVL) } {
			if { $ACB(DVL) == 0 } {
				# no devices
				set ACB(LDS) 0
				ac_again 1000
				return
			}
			set ACB(STA) "L"
			ac_again 100
			return
		}

		set dev [lindex $ACB(DVS) $ACB(CUR)]
		incr ACB(CUR)
		if { [$ACB(OPE) $dev] == 0 } {
			ac_again 100
			return
		}

		$ACB(HSH)

		set ACB(STA) "C"
		ac_again 2000
		return
	}

	if { $ACB(STA) == "C" } {
		# WAITING FOR HANDSHAKE
		if [$ACB(HSC)] {
			# established, assume connection OK
			set ACB(STA) "R"
			ac_again 2000
			return
		}
		# sorry, try another one
		# trc "AC C -> CLOSING"
		$ACB(CLO)
		set ACB(STA) "N"
		ac_again 100
		return
	}

	set ACB(STA) "L"
	ac_again 1000
}

namespace export autocn_*

}

namespace import ::AUTOCONNECT::*

###############################################################################
###############################################################################

proc cw { } {
#
# Returns the window currently in focus or null if this is the root window
#
	set w [focus]
	if { $w == "." } {
		set w ""
	}

	return $w
}

###############################################################################
# Modal windows ###############################################################
###############################################################################

proc md_click { val { lv 0 } } {
#
# Generic done event for modal windows/dialogs
#
	global Mod

	if { [info exists Mod($lv,EV)] && $Mod($lv,EV) == 0 } {
		set Mod($lv,EV) $val
	}

	trigger
}

proc md_stop { { lv 0 } } {
#
# Close operation for a modal window
#
	global Mod

	if [info exists Mod($lv,WI)] {
		catch { destroy $Mod($lv,WI) }
	}
	array unset Mod "$lv,*"
	# make sure all upper modal windows are destroyed as well; this is
	# in case grab doesn't work
	for { set l $lv } { $l < 10 } { incr l } {
		if [info exists Mod($l,WI)] {
			md_stop $l
		}
	}
	# if we are at level > 0 and previous level exists, make it grab the
	# pointers
	while { $lv > 0 } {
		incr lv -1
		if [info exists Mod($lv,WI)] {
			catch { grab $Mod($lv,WI) }
			break
		}
	}
}

proc md_wait { { lv 0 } } {
#
# Wait for an event on the modal dialog
#
	global Mod

	set Mod($lv,EV) 0

	event_loop

	if ![info exists Mod($lv,EV)] {
		return -1
	}
	if { $Mod($lv,EV) < 0 } {
		# cancellation
		md_stop $lv
		return -1
	}

	return $Mod($lv,EV)
}

proc md_window { tt { lv 0 } } {
#
# Creates a modal dialog
#
	global Mod

	set w [cw].modal$lv
	catch { destroy $w }
	set Mod($lv,WI) $w
	toplevel $w
	wm title $w $tt

	if { $lv > 0 } {
		set l [expr $lv - 1]
		if [info exists Mod($l,WI)] {
			# release the grab of the previous level window
			catch { grab release $Mod($l,WI) }
		}
	}

	# this fails sometimes
	catch { grab $w }
	return $w
}

###############################################################################
###############################################################################

proc alert { msg } {

	tk_dialog [cw].alert "Attention!" "${msg}!" "" 0 "OK"
}

proc confirm { msg } {

	set w [tk_dialog [cw].confirm "Warning!" $msg "" 0 "NO" "YES"]
	return $w
}

proc fparent { } {

	set w [focus]

	if { $w == "" } {
		return ""
	}

	return "-parent $w"
}

proc terminate { } {

	global DEAF ST PM

	if $DEAF { return }

	# catch { destroy . }

	set DEAF 1

	setsparams

	if $ST(SIP) {
		# send a reset request to the node
		send_reset 16
		wack 0xFE 0 $PM(SDL)
	}

	exit 0
}

proc strtail { str n } {
#
# Return no more than n trailing characters of the string
#
	if { $n < 4 } {
		set n 4
	}

	set ll [string length $str]

	if { $ll <= $n } {
		return $str
	}

	return "...[string range $str end-[expr {$n - 4}] end]"
}

proc isinteger { u } {

	if [catch { expr { int($u) } } v] {
		return 0
	}

	if [regexp -nocase "\[.e\]" $u] {
		return 0
	}

	return 1
}

proc intsize { v } {
#
# Returns the number of bytes needed to represent v
#
	if { $v < 0 } {
		set f -1
	} else {
		set f 0
	}

	set n 0
	while { $v != $f } {
		set v [expr $v >> 8]
		incr n
	}

	if { $n == 0 } {
		set n 1
	}

	return $n
}

proc int_to_bytes { v s } {
#
# Converts an int to s raw bytes in little-endian format
#
	set bb ""
	for { set n 0 } { $n < $s } { incr n } {
		set b [expr $v & 0xFF]
		set v [expr $v >> 8]
		append bb [binary format c $b]
	}
	return $bb
}

proc valid_int_number { n min max { res "" } } {

	if ![isinteger $n] {
		return 0
	}

	if { $n < $min || $n > $max } {
		return 0
	}

	if { $res != "" } {
		upvar $res r
		set r $n
	}

	return 1
}

proc valid_fp_number { n min max fra { res "" } } {

	if [catch { expr $n } val] {
		return 0
	}

	if [catch { format %1.${fra}f $n } val] {
		return 0
	}

	if { $val < $min || $val > $max } {
		return 0
	}

	if { $res != "" } {
		upvar $res r
		set r $val
	}

	return 1
}

proc valid_ban { n { res "" } } {
#
# Validates a band setting
#
	global PM

	if { $n != "Single" && $n != "Single/rcv" } {
		# neither of the two special values
		if [catch { expr $n } n] {
			return 0
		}
		set fa 1
		foreach p $PM(BAN) {
			if { [expr abs($p - $n)] < 0.0001 } {
				set n $p
				set fa 0
				break
			}
		}
		if $fa {
			return 0
		}
	}

	if { $res != "" } {
		upvar $res r
		set r $n
	}

	return 1
}

proc ishex { c } {
	return [regexp -nocase "\[0-9a-f\]" $c]
}

proc isoct { c } {
	return [regexp -nocase "\[0-7\]" $c]
}

proc stparse { line } {
#
# Parse a UNIX string into a list of hex codes; update the source to point to
# the first character behind the string
#
	upvar $line ln

	set nc [string index $ln 0]
	if { $nc != "\"" } {
		error "illegal string delimiter: $nc"
	}

	# the original - for errors
	set or "\""

	set ln [string range $ln 1 end]

	set vals ""

	while 1 {
		set nc [string index $ln 0]
		if { $nc == "" } {
			error "unterminated string: $or"
		}
		set ln [string range $ln 1 end]
		if { $nc == "\"" } {
			# done (this version assumes that the sentinel must be
			# explicit, append 0x00 if this is a problem)
			return $vals
		}
		if { $nc == "\\" } {
			# escapes
			set c [string index $ln 0]
			if { $c == "" } {
				# delimiter error, will be diagnosed at next
				# turn
				continue
			}
			if { $c == "x" } {
				# get hex digits
				set ln [string range $ln 1 end]
				while 1 {
					set d [string index $ln 0]
					if ![ishex $d] {
						break
					}
					append c $d
					set ln [string range $ln 1 end]
				}
				if [catch { expr 0$c % 256 } val] {
					error "illegal hex escape in string: $c"
				}
				lappend vals $val
				continue
			}
			if [isoct $c] {
				if { $c != 0 } {
					set c "0$c"
				}
				# get octal digits
				set ln [string range $ln 1 end]
				while 1 {
					set d [string index $ln 0]
					if ![isoct $d] {
						break
					}
					append c $d
					set ln [string range $ln 1 end]
				}
				if [catch { expr $c % 256 } val] {
					error \
					    "illegal octal escape in string: $c"
				}
				lappend vals $val
				continue
			}
			set ln [string range $ln 1 end]
			set nc $c
			continue
		}
		scan $nc %c val
		lappend vals [expr $val % 256]
	}
}

proc freq_decode { val } {

	global PM

	return [format %1.2f [expr (double ($val) / 65536.0) * $PM(FRE_qua)]]
}

proc freq_encode { freq } {

	global PM

	return [expr int( ($freq / $PM(FRE_qua)) * 65536.0 + 0.5)]
}

proc band_to_step { bf } {
#
# Converts band frequency to raw step
#
	global PM

	# half the scale
	set hb [expr ($PM(NSA) - 1) / 2]
	# band frequency step per sample
	set fi [expr $bf / double($hb)]
	# raw increment
	return [freq_encode $fi]
}

proc band_width { } {
#
# Returns the band interval <from to> (for a band scan)
#
	global RC

	set b $RC(BAN)
	set fr [expr $RC(FCE) - $b]
	set up [expr $RC(FCE) + $b]

	return [list $fr $up]
}

proc hencode { buf } {
#
# Encode binary buf as hex bytes
#
	set code ""

	set nb [string length $buf]
	binary scan $buf cu$nb code
	set ol ""

	foreach co $code {
		append ol [format " %02x" $co]
	}

	return $ol
}

proc getsparams { } {
#
# Retrieve parameters from the RC file
#
	global RC PM Params RGroups PLayouts PREINIT

	if { $PM(RCF) == "" } {
		# switched off
		return
	}

	if [catch { open $PM(RCF) "r" } fd] {
		# no such file
		if ![info exists PREINIT] {
			return
		}
		set pars $PREINIT
	} elseif [catch { read $fd } pars] {
		catch { close $fd }
		return
	} else {
		catch { close $fd }
	}

	# parameters
	foreach p $Params {
		if [catch { dict get $pars PARAMS $p } v] {
			continue
		}
		if ![info exists RC(+,$p)] {
			# something wrong
			continue
		}
		set valcode $RC(+,$p)
		if { $valcode == "" } {
			# no verification needed
			set RC($p) [string trim $v]
		} else {
			regsub "%I" $valcode "\$v" valcode
			regsub "%O" $valcode "v" valcode
			if [eval $valcode] {
				# OK
				set RC($p) $v
			}
		}
	}

	# register groups
	if [catch { dict keys [dict get $pars REGISTERS] } keys] {
		set keys ""
	}

	foreach ky $keys {
		if ![catch { dict get $pars REGISTERS $ky } item] {
			# note: an RG is a list, not a dictionary
			set RGroups($ky) $item
		}
	}

	if [info exists RGroups(Default)] {
		# redefining the built-in register group
		reguse Default
	}

	# inject layout database
	if [catch { dict keys [dict get $pars PLAYOUTS] } keys] {
		set keys ""
	}

	foreach ky $keys {
		if ![catch { dict get $pars PLAYOUTS $ky } item] {
			set PLayouts($ky) $item
		}
	}
}

proc setsparams { } {
#
# Save parameters in RC file
#
	global RC PM Params RGroups PLayouts

	if { $PM(RCF) == "" } {
		# disabled
		return
	}

	if [catch { open $PM(RCF) "w" } fd] {
		alert "Cannot save parameters in $PM(RCF), failed to open: $fd"
		set PM(RCF) ""
		return
	}

	set d [dict create]

	# parameters
	foreach p $Params {
		if ![info exists RC($p)] {
			# something wrong
			continue
		}
		dict set d PARAMS $p $RC($p)
	}

	# register groups
	foreach k [array names RGroups] {
		# do not save the special names
		if { [string index $k 0] == "+" } {
			continue
		}
		dict set d REGISTERS $k $RGroups($k)
	}

	# inject packet layouts
	foreach k [array names PLayouts] {
		dict set d PLAYOUTS $k $PLayouts($k)
	}

	if [catch { puts -nonewline $fd $d } err] {
		catch { close $fd }
		alert "Cannot save parameters in $PM(RCF),\
			failed to write: $err"
		set PM(RCF) ""
	}

	catch { close $fd }
}

###############################################################################

proc log { line } {

	global ST WN PM

	# the backing storage

	if { $ST(LOL) >= $PM(MXL) } {
		set de [expr $ST(LOL) - $PM(MXL) + 1]
		set ST(LOG) [lrange $ST(LOG) $de end]
		set ST(LOL) $PM(MXL)
	} else {
		incr ST(LOL)
	}

	lappend ST(LOG) $line

	# for brevity
	set t $WN(LOG)

	if { $t == "" } {
		# no window
		return
	}

	$t configure -state normal
	$t insert end "$line\n"

	while 1 {
		# remove excess lines from top
		set ix [$t index end]
		set ix [string range $ix 0 [expr [string first "." $ix] - 1]]
		if { $ix <= $PM(MXL) } {
			break
		}
		# delete the tompmost line
		$t delete 1.0 2.0
	}

	$t configure -state disabled
	$t yview -pickplace end

	if { $ST(LFD) != "" && [string index $line 0] != "*" } {
		# to file
		set sec [clock seconds]
		set day [clock format $sec -format %d]
		set hdr [clock format $sec -format "%H:%M:%S"]
		if { $day != $ST(LCD) } {
			# day change
			set today "Today is "
			append today [clock format $sec -format "%h $day, %Y"]
			if { $ST(LCD) == 0 } {
				# startup
				log_w "$hdr #### $today ####"
			} else {
				log_w "00:00:00 #### BIM! BOM! $today ####"
			}
			set ST(LCD) $day
		}
		log_w "$hdr $line"
	}
}

proc log_open { } {
#
# Opens the log window
#
	global ST WN FO RC

	if { $WN(LOG) != "" } {
		# already open
		return
	}

	set w ".logw"
	toplevel $w
	wm title $w "Log"

	set f [frame $w.lf]
	pack $f -side top -expand y -fill both
	set c $f.log

	set WN(LOG) [text $c -width 80 -height 24 -borderwidth 2 \
		-setgrid true -wrap none \
		-yscrollcommand "$f.scrolly set" \
		-xscrollcommand "$f.scrollx set" \
		-font $FO(LOG) \
		-exportselection 1 -state normal]

	scrollbar $f.scrolly -command "$c yview"
	scrollbar $f.scrollx -orient horizontal -command "$c xview"

	pack $f.scrolly -side right -fill y
	pack $f.scrollx -side bottom -fill x
	pack $c -side top -expand y -fill both

	set f [frame $w.bf]
	pack $f -side top -expand n -fill both

	button $f.c -text "Close" -command "log_toggle"
	pack $f.c -side right

	button $f.k -text "Clear" -command "log_clear"
	pack $f.k -side left

	#######################################################################

	set WN(LOF) [button $f.m -text "To file" -command "log_file_toggle"]
	pack $f.m -side left

	label $f.n -textvariable ST(LOF)
	pack $f.n -side left

	#######################################################################

	menubutton $f.as -text [lindex $WN(ASM) $RC(LON)] -direction right \
		-menu $f.as.m -relief raised
	menu $f.as.m -tearoff 0
	pack $f.as -side right -expand no -fill y

	set n 0
	foreach it $WN(ASM) {
		$f.as.m add command -label $it \
			-command "log_menu_click $f.as $n"
		incr n
	}

	label $f.p -text "Autostart: "
	pack $f.p -side right

	#######################################################################

	bind $w <Destroy> log_close
	bind $w <ButtonRelease-1> "tk_textCopy $w"
	bind $w <ButtonRelease-2> "tk_textPaste $w"

	$c delete 1.0 end
	$c configure -state normal

	# insert pending lines
	foreach ln $ST(LOG) {
		$c insert end "${ln}\n"
	}

	$c yview -pickplace end
	$c configure -state disabled

	$WN(LOB) configure -text "Packet Log Off"

	#######################################################################

	set ST(LOF) ""
	set ST(LFD) ""

	if { $RC(LON) == 2 } {
		# try to open the file
		if { $RC(LOF) == "" } {
			alert "No known log file, request ignored"
			set RC(LON) 1
		} else {
			if ![log_open_file $RC(LOF)] {
				# failure
				log_menu_click $f.as 1
			}
		}
	}
}

proc log_menu_click { wid n } {
#
# Invoked after a click in the autostart menu
#
	global RC WN ST

	set RC(LON) $n

	$wid configure -text [lindex $WN(ASM) $n]

	if { $n == 2 && $ST(LFD) == "" } {
		log_file_toggle
	}
}

proc log_clear { } {

	global WN

	if { $WN(LOG) == "" } {
		return
	}

	$WN(LOG) configure -state normal
	$WN(LOG) delete 1.0 end
	$WN(LOG) configure -state disabled
}

proc log_close { } {

	global WN

	log_close_file
	catch { destroy .logw }
	set WN(LOG) ""
	$WN(LOB) configure -text "Packet Log On"
}

proc log_close_file { } {

	global ST WN

	if { $ST(LFD) != "" } {
		# close the log file
		catch { close $ST(LFD) }
		set ST(LFD) ""
		set ST(LOF) ""
		catch { $WN(LOF) configure -text "To file" }
	}
}

proc log_open_file { fn } {

	global ST RC WN

	if [catch { open $fn "a" } fd] {
		alert "Cannot open log file $fn, $fd"
		return 0
	}

	# last day log written
	set ST(LCD) 0

	# assume everything's OK
	set ST(LOF) [strtail $fn 32]
	set ST(LFD) $fd
	$WN(LOF) configure -text "Stop file"
	set RC(LOF) $fn

	log "@@ file started"

	return 1
}

proc log_toggle { } {
#
# Close/open the log window
#
	global WN

	if { $WN(LOG) == "" } {
		log_open
	} else {
		log_close
	}
}

proc log_file_toggle { } {
#
# Closes/opens the log file
#
	global ST RC

	if { $ST(LFD) != "" } {
		log_close_file
		return
	}

	set ou $RC(LOF)

	while 1 {
		set fn [tk_getSaveFile \
			-defaultextension ".log" \
			{*}[fparent] \
			-title "Log file name" \
			-initialfile [file tail $ou] \
			-initialdir [file dirname $ou]]

		if { $fn == "" } {
			# cancelled
			return
		}

		if [log_open_file $fn] {
			return
		}
	}
}

proc log_w { msg } {

	global ST

	catch {
		puts $ST(LFD) $msg
		flush $ST(LFD)
	}
}

proc log_auto { } {
#
# Called to check if the log window should be auto-called at this moment
#
	global RC WN 

	if { $RC(LON) && [mode_reception] && $WN(LOG) == "" } {
		log_open
	}
}
	
###############################################################################

proc rqc { } {
#
# New request sequence number
#
	global ST

	incr ST(RQC)

	if { $ST(RQC) > 255 } {
		set ST(RQC) 0
	}

	return $ST(RQC)
}

proc send_reset { { n 2 } } {

	set t [binary format cc 0xFE 0]

	while { $n } {
		noss_send $t
		incr n -1
	}
}

proc send_handshake { } {

	send_reset 32
}

proc send_poll { } {

	noss_send [binary format cc 0x05 0]
}

proc handshake_ok { } {

	global ST

	return $ST(HSK)
}

###############################################################################

proc uart_open { udev } {

	global PM ST

	set emu [uartpoll_interval $ST(SYS) $ST(DEV)]
	if { $ST(SYS) == "L" } {
		set accs { RDWR NOCTTY NONBLOCK }
	} else {
		set accs "r+"
	}

	set d [unames_unesc $udev]
	# trc "opening $d $accs"

	if [catch { open $d $accs } ST(SFD)] {
		# trc "failed: $ST(SFD)"
		set ST(SFD) ""
		return 0
	}

	if [catch { fconfigure $ST(SFD) -mode "$PM(DSP),n,8,1" -handshake none \
	    -blocking 0 -eofchar "" -ttycontrol { RTS 0 } } err] {
		catch { close $ST(SFD) }
		set ST(SFD) ""
		return 0
	}

	set ST(UDV) $udev

	# handshake condition not met
	set_status "handshake $ST(UDV)"
	set ST(HSK) 0

	# configure the protocol
	noss_init $ST(SFD) $PM(MPL) uart_first_read "" uart_close $emu

	return 1
}

proc uart_close { { err "" } } {

	global ST

	# trc "close UART: $err"

	if $ST(SIP) {
		send_reset 8
	}

	catch { close $ST(SFD) }
	set ST(SFD) ""

	clear_sip

	set_status "disconnected"

	trigger
}

proc uart_connected { } {

	global ST

	return [expr { $ST(SFD) != "" }]
}

proc uart_first_read { msg } {
#
# To establish a handshake (receive a magic byte) from the node
#
	global ST PM

	# dmp "FIR" $msg
	if { [string length $msg] < 4 } {
		return
	}

	binary scan $msg cucucucu a b c d
	# trc "handshake: $a $b $c $d"

	if { $a != 0x05 || $b != 0xCE || $c != 0x31 || $d != 0x7A } {
		# still not it
		return
	}

	# handshake condition met
	set ST(HSK) 1
	set_status "connected $ST(UDV)"
	noss_oninput uart_read

	clear_sip
	update_widgets

	trigger

	after 0 try_start
}

proc ack_timeout { } {
#
# Timeout for an expected command
#
	global ST

	set ST(ECM,C) -1
	trigger
}

proc wack { cmd seq { tim 8000 } } {
#
# Wait for a response
#
	global ST PM

	set ST(ECM) $cmd
	set ST(ECM,S) $seq

	set ST(ECM,C) 0
	set ST(ECM,M) ""
	set ST(ECM,T) [after $tim ack_timeout]

	while 1 {
		event_loop
		if { $ST(ECM,C) != 0 } {
			catch { after cancel $ST(ECM,T) }
			return $ST(ECM,C)
		}
	}
}

proc uart_read { msg } {

	global ST PM DEAF

	autocn_heartbeat

	set len [expr [string length $msg] - 2]
	binary scan $msg cucu code seq

	foreach c $ST(ECM) {
		if { $code == $c && $ST(ECM,S) == $seq } {
			# this command has been expected
			set ST(ECM,C) $c
			set ST(ECM,M) [string range $msg 2 end]
			trigger
			return
		}
	}

	if $DEAF {
		# closing; ignore any input even if the UART is still open; it
		# can be, because we want to (try to) stop the node
		return
	}

	if { $code == 2 } {
		if { $len < $PM(NSA) } {
			# trc "Bad sample, $len bytes instead of $PM(NSA)"
			return
		}
		set msg [string range $msg 2 end]
		if { $ST(SIP) == 1 } {
			process_band_sample $msg
		} elseif { $ST(SIP) != 0 } {
			process_freq_sample $msg
		}
		return
	}

	if { $code == 1 } {
		# packet
		log_packet [string range $msg 2 end] $len
		return
	}
}

proc tstamp { } {

	return [clock format [clock seconds] -format "%H:%M:%S"]
}

proc log_packet { smp len } {

	if { $len < 5 } {
		return
	}

	binary scan $smp cucu l0 ln
	set smp [string range $smp 2 end]
	set l0 [string length $smp]
	if { $l0 > [expr $ln + 2] } {
		set smp [string range $smp 0 [expr $ln + 1]]
	}
	binary scan [string index $smp [expr [string length $smp] - 2]] cu rss
	set rss [expr { ($rss + 128) & 0xff }]
	log "** received packet \[[tstamp]\] (length=$ln, rssi=$rss):"
	log "<= [string trimleft [hencode $smp]]"
}

proc process_band_sample { smp } {

	global PM RC SM MX

	# calculate new curve
	set alpha [expr 2.0/double(1 + $RC(EMA))]

	binary scan $smp cu$PM(NSA) code

	set i 0
	foreach v $code {
		set SM($i) [expr $v * $alpha + (1.0 - $alpha) * $SM($i)]
		if { $SM($i) > $MX($i) } {
			set MX($i) $SM($i)
			if { $SM($i) > $MX(P,V) } {
				set MX(P,V) $SM($i)
				set MX(P,F) $i
			}
		}
		incr i
	}

	clear_sample
	draw_band_sample
	if $RC(TMX) {
		draw_band_max
	}
}

proc process_freq_sample { smp } {

	global PM RC SM MX

	binary scan $smp cu$PM(NSS) code

	set max 0
	set fr 0
	set ro [expr $RC(MOA) / 2]

	while 1 {

		if { $fr >= $PM(NSS) } {
			break
		}

		set to [expr $fr + $RC(MOA)]

		if { $to > $PM(NSS) } {
			# shift back
			set d [expr $to - $PM(NSS)]
			incr fr -$d
			incr to -$d
		}

		set v 0
		while { $fr < $to } {
			incr v [lindex $code $fr]
			incr fr
		}

		set v [expr ($v + $ro) / $RC(MOA)]

		if { $v > $max } {
			set max $v
		}
	}

	set in $SM(IN)
	set SM($in) $max
	incr in
	if { $in == $PM(NSS) } {
		set in 0
	}
	set SM(IN) $in

	if { $max > $MX(0) } {
		set MX(0) $max
	}

	clear_sample
	draw_freq_sample
	if $RC(TMX) {
		draw_freq_max
	}
}

proc pack3 { val } {
#
# Packs a three-byte value
#
	return [binary format ccc [expr  $val        & 0xff] \
				  [expr ($val >>  8) & 0xFF] \
				  [expr ($val >> 16) & 0xFF] \
	       ]
}

proc pack2 { val } {
#
# Packs a three-byte value
#
	return [binary format cc [expr  $val        & 0xff] \
				 [expr ($val >>  8) & 0xFF] \
	       ]
}

proc pack1 { val } {

	return [binary format c [expr  $val & 0xff]]
}

proc clear_sample { } {

	global WN SM

	$WN(CAN) delete smp
}

proc clear_sip { } {
#
# Enter the idle state after re-connection
#
	global ST WN PM

	if $ST(SIP) {
		set ST(SIP) 0
		$WN(SSB) configure -text "Start"
		update_widgets
		inject_button_status
	}
}

proc try_start { args } {
#
# Try to start up when the autostart checkbutton becomes checked
#
	global ST RC

	if { $RC(AST) && $ST(SFD) != 0 && !$ST(SIP) } {
		do_start_stop
	}
}

proc do_start_stop { } {

	global ST SM MX RC PM WN

	if { $ST(SFD) == "" } {
		# disconnected
		return
	}

	if $ST(SIP) {
		# stop scan
		send_reset 96
		if { [wack 0xFE 0] < 0 } {
			alert "Node reset timeout"
			return
		}
		clear_sip
		return
	}

	clear_sample

	set fc [freq_encode $RC(FCE)]

	if [mode_single] {
		if [mode_reception] {
			set mo 2
		} else {
			set mo 1
		}
		set fs 0
	} else {
		set mo 0
		# calculate step frequency
		set fs [band_to_step $RC(BAN)]
		set fc [expr $fc - ($PM(NSS)/2) * $fs]
	}

	set SM(fc) $fc
	set SM(fs) $fs
	set SM(mo) $mo

	# trc "SEP: $mo, $RC(FCE)<$fc>, $RC(BAN)<$fs>, $RC(STA), $RC(ISD)"

	# reset the node
	send_reset 8
	if { [wack 0xFE 0] < 0 } {
		alert "Node reset timeout"
		return
	}

	# write registers
	set err [regs_write]
	if { $err != "" } {
		alert $err
		return
	}

	# issue the request
	set seq [rqc]
	set cmd [binary format cc 0x02 $seq]

	append cmd [pack1 $mo]
	append cmd [pack3 $fc]
	append cmd [pack3 $fs]
	append cmd [pack1 $RC(STA)]
	append cmd [pack1 $RC(ISD)]

	noss_send $cmd
	noss_send $cmd
	noss_send $cmd

	set ST(SIP) [expr $mo + 1]
	$WN(SSB) configure -text "Stop"

	for { set i 0 } { $i < $PM(NSA) } { incr i } {
		set SM($i) $RC(GRS)
	}

	reset_max

	# this is the end-buffer pointer for freq samples
	set SM(IN) 0

	update_widgets
	inject_button_status
}

###############################################################################

proc all_false { } { return 0 }

proc nsbox { w lab min max var row { lst "" } } {
#
# Create a numerical spinbox
#
	global WN PM

	label ${w}l -text "$lab:    " -anchor w
	grid ${w}l -column 0 -row $row -sticky sw

	set b ${w}b

	if { [expr $max - $min] > 15 } {
		# use a slider
		scale $b -orient horizontal -from $min -to $max -resolution 1 \
			-variable RC($var) -showvalue 1 \
			-command "do_spinbox $var"
	} else {
		# use a spinbox with no entry capabilities
		spinbox $b -from $min -to $max -textvariable RC($var) \
		-validate all -vcmd "all_false" \
		-justify right -command "do_spinbox $var"
	}

	if { $lst != "" } {
		lappend WN($lst) $b
	}

	grid $b -column 1 -row $row -sticky news

	return $b
}

proc graph_bounds { } {
#
# Calculate drawing parameters implied by the canvas size (which we may want
# to make resizable in the future) and the current numerical parameters
#
	global WN RC

	# starting x-coordinate
	set xs $WN(LMA)

	# ending x-coordinate
	set xe [expr $WN(CAW) + $xs]

	# starting y-coordinate
	set ys $WN(BMA)

	# ending y-coordinate
	set ye [expr $WN(CAH) + $ys]

	set WN(xs) $xs
	set WN(xe) $xe
	set WN(ys) $ys
	set WN(ye) $ye
	set WN(yd) [expr $ye - $ys + 1]
	# complete height of the canvas for mirroring
	set WN(ym) [expr $WN(CAH) + $WN(BMA) + $WN(UMA)]
	# the middle x point
	set WN(xh) [expr ($WN(xs) + $WN(xe)) / 2]
}

proc yco { y } {
#
# Mirror the y coordinate
#
	global WN

	return [expr $WN(ym) - $y]
}

proc draw_x_tick { x val } {

	global WN FO

	set y0 [yco [expr $WN(ys) - $WN(XTS)]]

	$WN(CAN) create line $x [yco $WN(ys)] $x $y0 \
		-fill $WN(AXC) \
		-tags axis

	if { $x == $WN(xs) } {
		set an "nw"
	} elseif { $x == $WN(xe) } {
		set an "ne"
	} else {
		set an "n"
	}
	$WN(CAN) create text $x $y0 -text [format %1.2f $val] -anchor $an \
		-font $FO(SMA) -fill $WN(AXC) -tags axis
}

proc draw_x_recs { lev xl xr vl vr } {
#
# Draw x-ticks recursively
#
	global WN

	if { $lev >= $WN(ml) } {
		# maximum depth reached
		return
	}

	set d [expr ($xr + $xl) / 2]
	set v [expr $vl + (double($d - $xl)/($xr - $xl - 1)) * ($vr - $vl)]

	draw_x_tick $d $v

	incr lev
	if { $lev == $WN(ml) } {
		return
	}

	draw_x_recs $lev $xl $d $vl $v
	draw_x_recs $lev $d $xr $v $vr
}

proc draw_y_ticks { xc } {
#
# Draw y ticks along the vertical axis
#
	global ST WN RC FO

	if { $xc == $WN(xh) } {
		# center line
		set x0 [expr $xc - $WN(YTS)]
		set x1 [expr $xc + $WN(YTS)]
		set xt $x1
		set tan "w"
	} else {
		# right line
		set x0 [expr $xc - $WN(YTS) - $WN(YTS)]
		set x1 [expr $xc]
		set xt $x0
		set tan "e"
	}

	set D [expr $RC(MRS) - $RC(GRS)]
	set nv [expr $D / 8.0]

	foreach ns { 1 2 5 10 15 20 25 30 50 } {
		# calculate the step
		if { $ns > $nv } {
			break
		}
	}

	if { $ns == 1 } {
		set nt 1
	} else {
		set nt [expr $ns / 2]
	}

	set DM [expr $D - $nt]

	if $ST(SSM) {
		# DB
		for { set y $ns } { $y <= $DM } { incr y $ns } {
			set ya [stoy [expr $y + $RC(GRS)]]
			$WN(CAN) create line $x0 $ya $x1 $ya -fill $WN(AXC) \
				-tags axis
			$WN(CAN) create text $xt $ya -text \
				[format " %1d dB" $y] -anchor $tan \
				-font $FO(SMA) -fill $WN(AXC) -tags axis
		}
	} else {
		# RSSI
		set st [expr (($RC(GRS) + $nt) / $ns) * $ns]
		if { [expr $st - $RC(GRS)] < $nt } {
			set st [expr $st + $ns]
		}
		set DM [expr $DM + $RC(GRS)]
		for { set y $st } { $y <= $DM } { incr y $ns } {
			set ya [stoy $y]
			$WN(CAN) create line $x0 $ya $x1 $ya -fill $WN(AXC) \
				-tags axis
			$WN(CAN) create text $xt $ya -text \
				[format " %1d" $y] -anchor $tan \
				-font $FO(SMA) -fill $WN(AXC) -tags axis
		}
	}
}

proc draw_axes { } {

	global WN RC FO

	# the canvas
	set cv $WN(CAN)

	# the color
	set co $WN(AXC)

	# calculate the frame
	graph_bounds

	$WN(CAN) delete axis

	# horizontal axis
	set y0 [yco $WN(ys)]
	$WN(CAN) create line $WN(xs) $y0 $WN(xe) $y0 -fill $WN(AXC) \
		-width 2 -tags axis

	if [mode_single] {
		draw_x_tick $WN(xe) $RC(FCE)
		# the vertical axis
		$WN(CAN) create line $WN(xe) $y0 $WN(xe) [yco $WN(ye)] \
			-fill $WN(AXC) -width 1 -tags axis
		draw_y_ticks $WN(xe)
	} else {
		# two edge ticks
		lassign [band_width] fr up
		draw_x_tick $WN(xs) $fr
		draw_x_tick $WN(xe) $up
		# calculate the maximum level of ticks
		set lev 0
		set spa [expr $WN(xe) - $WN(xs)]
		while { $spa > $WN(MHT) } {
			incr lev
			set spa [expr $spa / 2]
		}
		set WN(ml) $lev
		# recurse
		draw_x_recs 0 $WN(xs) $WN(xe) $fr $up
		# the vertical axis
		$WN(CAN) create line $WN(xh) $y0 $WN(xh) [yco $WN(ye)] \
			-fill $WN(AXC) -width 1 -tags axis
		draw_y_ticks $WN(xh)
	}
}

proc stoy { v } {
#
# Converts RSSI sample into graph y
#
	global RC WN

	return [yco [expr round((double($v - $RC(GRS))/($RC(MRS) - $RC(GRS))) \
	 * $WN(yd)) + $WN(ys)]]
}

proc draw_freq_sample { } {
#
	global WN SM RC PM

	set xa [expr $WN(xs) + $WN(SEF) - 1]
	set ix $SM(IN)
	set yp 0
	for { set i 0 } { $i < $PM(NSS) } { incr i } {
		set xb [expr $xa + $WN(SEF)]
		set v $SM($ix)
		incr ix
		if { $ix == $PM(NSS) } {
			set ix 0
		}
		if { $v < $RC(GRS) } {
			set v $RC(GRS)
		} elseif { $v > $RC(MRS) } {
			set v $RC(MRS)
		}

		set y [stoy $v]

		if { $yp > 0 } {
			$WN(CAN) create line $xa $yp $xa $y $xb $y \
				-fill $WN(SMC) -width 1 -tags smp
		} else {
			$WN(CAN) create line $xa $y $xb $y \
				-fill $WN(SMC) -width 1 -tags smp
		}
		set xa $xb
		set yp $y
	}
}

proc draw_freq_max { } {
#
	global WN MX RC PM

	set v $MX(0)

	if { $v < $RC(GRS) } {
		set v $RC(GRS)
	} elseif { $v > $RC(MRS) } {
		set v $RC(MRS)
	}
	set y [stoy $v]

	$WN(CAN) create line $WN(xs) $y $WN(xe) $y -fill $WN(MXC) \
		-width 1 -tags smp
}

proc draw_band_sample { } {
#
	global WN SM RC PM

	set xa $WN(xs)
	for { set i 0 } { $i < $PM(NSA) } { incr i } {
		set xb [expr $xa + $WN(SEF)]
		set v $SM($i)
		if { $v < $RC(GRS) } {
			set v $RC(GRS)
		} elseif { $v > $RC(MRS) } {
			set v $RC(MRS)
		}
		set y [stoy $v]
		$WN(CAN) create line $xa $y $xb $y -fill $WN(SMC) -width 2 \
			-tags smp
		set xa $xb
	}
}

proc draw_band_max { } {
#
	global WN MX RC PM FO

	set xa $WN(xs)
	set yp 0
	for { set i 0 } { $i < $PM(NSA) } { incr i } {
		set xb [expr $xa + $WN(SEF)]
		set v $MX($i)
		if { $v < $RC(GRS) } {
			set v $RC(GRS)
		} elseif { $v > $RC(MRS) } {
			set v $RC(MRS)
		}
		set y [stoy $v]
		if { $yp > 0 } {
			$WN(CAN) create line $xa $yp $xa $y $xb $y \
				-fill $WN(MXC) -width 1 -tags smp
		} else {
			$WN(CAN) create line $xa $y $xb $y \
				-fill $WN(MXC) -width 1 -tags smp
		}
		set xa $xb
		set yp $y
	}

	if { $MX(P,V) != "" } {
		# show the current peak
		lassign [band_width] fr up
		set f [expr ((($MX(P,F) + 0.5) * ($up - $fr)) / $PM(NSA)) + $fr]
		$WN(CAN) create text $WN(PXF) $WN(PYF) -text \
			[format "Pk: %1d/%1.2f" [expr round($MX(P,V))] $f] \
			-fill $WN(PCO) -tags smp \
			-anchor w
	}
}

proc reset_max { } {

	global MX PM

	for { set i 0 } { $i < $PM(NSA) } { incr i } {
		set MX($i) 0
		set MX(P,V) ""
		set MX(P,F) ""
	}
}

proc redraw { } {

	global SM

	clear_sample
	draw_axes
}

proc update_widgets { } {
#
# Enable/disables the widgets whose status depends on our state
#
	global WN ST

	if { $ST(SFD) == "" } {
		# not connected, not scanning
		set sm "disabled"
		set sc "disabled"
		set ss "normal"
	} else {
		set sm "normal"
		set sc "normal"
		if $ST(SIP) {
			# scanning
			set ss "disabled"
		} else {
			set ss "normal"
		}
	}

	$WN(SSB) configure -state $sm

	foreach c [array names WN "*,SIP"] {
		foreach w $WN($c) {
			$w configure -state $ss
		}
	}

	foreach c [array names WN "*,CON"] {
		foreach w $WN($c) {
			$w configure -state $sc
		}
	}
}

proc do_spinbox { var { val "" } } {

	global RC

	if { $var == "GRS" || $var == "MRS" } {
		redraw
	}
}

proc mode_single { } {

	global RC

	if { [string index $RC(BAN) 0] == "S" } {
		return 1
	}

	return 0
}

proc mode_reception { } {

	global RC

	if { [string first "rcv" $RC(BAN)] < 0 } {
		return 0
	}

	return 1
}

proc set_averaging_button { } {
#
# Updates the text on the "Averaging" button
#
	global WN RC

	if [mode_single] {
		set t "MOA=$RC(MOA)"
	} else {
		set t "EMA=$RC(EMA)"
	}

	$WN(AVB) configure -text $t
}

proc fix_min_isd { } {
#
# Fixes the minimum "inter-sample delay" depending on the mode
#
	global WN RC

	if [mode_single] {
		set min 0
	} else {
		set min 1
		if { $RC(ISD) == 0 } {
			set RC(ISD) 1
		}
	}

	$WN(ISB) configure -from $min
}

proc set_sscale_button { } {

	global ST WN

	if $ST(SSM) {
		set t "dB"
	} else {
		set t "RSSI"
	}
	
	$WN(SGB) configure -text $t
}

proc set_sscale { } {

	global ST

	if $ST(SSM) {
		set ST(SSM) 0
	} else {
		set ST(SSM) 1
	}

	set_sscale_button
	redraw
}

proc band_menu_click { w n } {
#
# Handles a selection from the band menu
#
	global RC

	set t [$w.m entrycget $n -label]
	$w configure -text $t
	set RC(BAN) $t
	set_averaging_button
	fix_min_isd
	redraw
	log_auto
}

proc set_averaging { } {
#
# Sets one of the three frequency parameters
#
	global RC Mod WN PM

	# create the window
	set w [md_window "Set averaging mode"]

	if [mode_single] {
		set var "MOA"
		set max $PM(MOA_max)
	} else {
		set var "EMA"
		set max $PM(EMA_max)
	}

	set Mod(0,CU) $RC($var)

	set f $w.tf
	frame $f
	pack $f -side top -expand y -fill x

	if { $var == "MOA" } {
		set tt "Maximum of averages of ..."
	} else {
		set tt "Exponential moving average of ..."
	}

	label $f.ll -text $tt
	pack $f.ll -side top

	set sli $w.sc

	scale $sli -orient horizontal -length $WN(SLW) \
		-from 1 -to $max -resolution 1 \
		-tickinterval [expr $max/8] \
		-variable Mod(0,CU) \
		-showvalue 1

	pack $sli -side top -expand y -fill x

	# frame for buttons
	set f $w.bf
	frame $f
	pack $f -side top -expand y -fill x

	button $f.d -text "Done" -command "md_click 1"
	pack $f.d -side right -expand n

	button $f.c -text "Cancel" -command "md_click -1"
	pack $f.c -side left -expand n

	bind $w <Destroy> "md_click -1"

	#######################################################################

	while 1 {

		set ev [md_wait]

		if { $ev < 0 } {
			# cancellation
			return
		}

		if { $ev == 1 } {
			# set the changes
			set RC($var) $Mod(0,CU)
			md_stop
			set_averaging_button
			return
		}
	}
}

proc set_center_frequency { w } {
#
# Sets one of the three frequency parameters
#
	global RC Mod WN PM

	# create the window
	set w [md_window "Set center frequency"]

	set Mod(0,CU) $RC(FCE)

	# the slider is a frame of its own
	set sli $w.sc

	scale $sli -orient horizontal -length $WN(SLW) \
		-from $PM(FRE_min) -to $PM(FRE_max) -resolution 0.01 \
		-tickinterval [expr ($PM(FRE_max) - $PM(FRE_min)) / 6] \
		-variable Mod(0,CU) \
		-showvalue 1

	pack $sli -side top -expand y -fill x

	# frame for buttons
	set f $w.bf
	frame $f
	pack $f -side top -expand y -fill x

	button $f.d -text "Done" -command "md_click 1"
	pack $f.d -side right -expand n

	button $f.c -text "Cancel" -command "md_click -1"
	pack $f.c -side left -expand n

	bind $w <Destroy> "md_click -1"

	#######################################################################

	while 1 {

		set ev [md_wait]

		if { $ev < 0 } {
			# cancellation
			return
		}

		if { $ev == 1 } {
			# set the changes
			set RC(FCE) $Mod(0,CU)
			md_stop
			redraw
			return
		}
	}
}

proc mk_rootwin { win } {

	global WN ST RC PM

	wm title $win "Olsonet Spectrum Analyzer V$ST(VER)"

	if { $win == "." } {
		set w ""
	} else {
		set w $win
	}

	# list of scan parameter widgets
	set WN(RWI,SIP) ""

	set WN(CAN) "$w.c"
	canvas $WN(CAN) -width [expr $WN(CAW) + $WN(LMA) + $WN(RMA)] \
		       -height [expr $WN(CAH) + $WN(BMA) + $WN(UMA)] \
				-bg black
	pack $WN(CAN) -side left -expand n

	set mf "$w.tf"
	frame $mf
	pack $mf -side left -expand y -fill both

	#######################################################################

	set f $mf.t
	frame $f -pady 4 -padx 4
	pack $f -side top -expand y -fill x -anchor n

	#######################################################################

	set x $f.t

	labelframe $x -padx 4 -pady 4 -text "Scan parameters"
	pack $x -side top -expand y -fill both -anchor w

	#######################################################################

	label $x.cl -text "Center frequency (MHz): " -anchor w
	grid $x.cl -column 0 -row 0 -sticky nsw

	button $x.cb -textvariable RC(FCE) -anchor c \
		-command "set_center_frequency $x.cb"

	lappend WN(RWI,SIP) $x.cb
	grid $x.cb -column 1 -row 0 -sticky news

	#######################################################################

	label $x.bl -text "Band (MHz) / mode: " -anchor w
	grid $x.bl -column 0 -row 1 -sticky nsw

	set ol [concat { "Single" "Single/rcv" } $PM(BAN)]
	menubutton $x.bm -text $RC(BAN) -direction left -menu $x.bm.m \
		-relief raised
	menu $x.bm.m -tearoff 0
	set n 0
	foreach it $ol {
		$x.bm.m add command -label $it \
			-command "band_menu_click $x.bm $n"
		incr n
	}
	lappend WN(RWI,SIP) $x.bm
	grid $x.bm -column 1 -row 1 -sticky news

	#######################################################################

	nsbox $x.sa "Samples to average" 1 $PM(STA_max) STA 2 RWI,SIP

	set WN(ISB) [nsbox $x.is "Inter-sample delay" 0 $PM(ISD_max) ISD 3 \
		RWI,SIP]

	#######################################################################

	grid columnconfigure $x 0 -weight 0
	grid columnconfigure $x 1 -weight 1

	#######################################################################

	set x $f.gr

	labelframe $x -padx 4 -pady 4 -text "Graph parameters"
	pack $x -side top -expand y -fill both -anchor w

	#######################################################################

	nsbox $x.ba "Ground RSSI" 0 128 GRS 0
	nsbox $x.ma "Maximum RSSI" 128 255 MRS 1

	#######################################################################

	label $x.al -text "Averaging:  "
	grid $x.al -column 0 -row 2 -sticky nsw

	button $x.ab -text "-" -anchor c -command "set_averaging"
	set WN(AVB) $x.ab
	set_averaging_button
	fix_min_isd
	grid $x.ab -column 1 -row 2 -sticky news

	#######################################################################

	label $x.gl -text "Signal scale:  "
	grid $x.gl -column 0 -row 3 -sticky nsw

	button $x.gb -text "-" -anchor c -command "set_sscale"
	set WN(SGB) $x.gb
	set_sscale_button
	grid $x.gb -column 1 -row 3 -sticky news

	#######################################################################

	label $x.ml -text "Track maximum:  "
	grid $x.ml -column 0 -row 4 -sticky nsw

	frame $x.uf
	grid $x.uf -column 1 -row 4 -sticky news
	
	checkbutton $x.uf.mb -variable RC(TMX)
	pack $x.uf.mb -side left

	button $x.uf.mc -text Rst -command reset_max
	pack $x.uf.mc -side right -fill x -expand 1

	#######################################################################

	grid columnconfigure $x 0 -weight 0
	grid columnconfigure $x 1 -weight 1

	#######################################################################

	set x $f.ub
	frame $x -relief sunken
	pack $x -side top -expand y -fill x -anchor n

	grid columnconfigure $x 0 -weight 0
	grid columnconfigure $x 1 -weight 1

	#######################################################################
	button $x.lo -text "Packet Log On" -command "log_toggle" -width 12
	grid $x.lo -column 0 -row 0 -sticky news
	set WN(LOB) $x.lo
	#######################################################################

	#######################################################################
	button $x.rg -text "Show Regs" -command "regs_toggle" -width 8
	grid $x.rg -column 0 -row 1 -sticky news
	set WN(REB) $x.rg
	#######################################################################

	frame $x.af -padx 4 -pady 4
	grid $x.af -column 1 -row 0 -sticky ne

	label $x.af.h -text "Autostart: " -anchor e
	pack $x.af.h -side left -expand n
	checkbutton $x.af.c -variable RC(AST)
	pack $x.af.c -side right -expand n
	trace add variable RC(AST) write try_start

	#######################################################################

	set x $mf.bu

	frame $x -padx 4 -pady 4 -relief ridge
	pack $x -side bottom -expand y -fill x -anchor s

	#######################################################################
	button $x.qu -text "Quit" -command "terminate"
	pack $x.qu -side left
	#######################################################################

	#######################################################################
	button $x.go -text "Start" -command "do_start_stop"
	pack $x.go -side right
	set WN(SSB) $x.go
	#######################################################################

	#######################################################################
	button $x.in -text "Inject" -command "inject_click"
	pack $x.in -side right
	#######################################################################

	bind . <Destroy> "terminate"

	# check if should summon the log window
	log_auto
}

###############################################################################
# Registers ###################################################################
###############################################################################

# the always-present default, built-in register groups; RO registers occur as
# a special group
set RGroups(+RW) ""
set RGroups(+RO) ""

proc regdef { name addr def { len 0 } { status 0 } } {
#
# Defines a register
#
	global RG RGroups

	set RG($name,A) [expr $addr]
	set RG($name,L) $len
	# for status (read-only) registers
	set RG($name,S) $status

	if $status {
		lappend RGroups(+RO) $name
		set addr [expr $addr | 0xC0]
	} else {
		# only RW registers go to groups
		lappend RGroups(+RW) [list $name $def]
	}

	if !$len {
		# only single-valued registers can have fields
		set RG($name,NF) 0
	}

	regset $name $def

	set RG($name,T) ""
}

proc regtip { name tip } {

	global RG

	set RG($name,T) $tip
}

proc regfld { name tag h { l "" } } {
#
# Add a field to a reg definition
#
	global RG

	if { $l == "" } {
		set l $h
	}

	if $RG($name,L) {
		error "fields not supported for multivalue registers"
	}

	set x $RG($name,NF)
	incr RG($name,NF)

	set RG($name,F$x,n) $tag
	set RG($name,F$x,h) $h
	set RG($name,F$x,l) $l
	set RG($name,F$x,t) ""
}

proc regftp { name tag tip } {

	global RG

	set n $RG($name,NF)

	for { set i 0 } { $i < $n } { incr i } {
		if { $RG($name,F$i,n) == $tag } {
			set RG($name,F$i,t) $tip
			return
		}
	}

	error "field $tag not found in $name"
}

proc regval { name } {
#
# Returns register value
#
	global RG FO

	if ![info exists RG($name,A)] {
		return ""
	}

	if $RG($name,L) {
		# multivalue
		set res ""
		for { set i 0 } { $i < $RG($name,L) } { incr i } {
			lappend res [expr 0x$RG($name,V,$i)]
		}
		return $res
	}

	return [expr 0x$RG($name,V)]
}

proc regwid { name w row } {
#
# Sets up a register widget
#
	global RG WN BV FO

	label $w.n$row -text $name -anchor w
	grid $w.n$row -column 0 -row $row -sticky nes
	if { $RG($name,T) != "" } {
		tip_set $w.n$row $RG($name,T)
	}

	set f $w.f$row
	frame $f -borderwidth 1
	grid $f -column 1 -row $row -sticky news

	# read only?
	set ro $RG($name,S)

	# value length
	set ln $RG($name,L)

	# value label color
	if $ro {
		set lc $WN(RRC)
	} else {
		set lc $WN(RWC)
	}

	if $ln {
		# a multi-valued register, no fields
		for { set i 0 } { $i < $ln } { incr i } {
			label $f.tl$i -text " V$i:"
			pack $f.tl$i -side left
			label $f.td$i -textvariable RG($name,V,$i) -bg $lc \
				-font $FO(REG) -cursor $WN(CCD)
			pack $f.td$i -side left
			if !$ro {
				bind $f.td$i <ButtonRelease-1> \
				    "reg_val_change $name +1 $f.td$i %x $i"
				bind $f.td$i <ButtonRelease-2> \
				    "reg_val_change $name -1 $f.td$i %x $i"
				bind $f.td$i <ButtonRelease-3> \
				    "reg_val_change $name -1 $f.td$i %x $i"
			}
		}
		# no fields
		set ns 0

	} else {

		# single value
		label $f.td -textvariable RG($name,V) -bg $lc -font $FO(REG) \
			-cursor $WN(CCD)
		pack $f.td -side left
		if !$ro {
			bind $f.td <ButtonRelease-1> \
				"reg_val_change $name +1 $f.td %x"
			bind $f.td <ButtonRelease-2> \
				"reg_val_change $name -1 $f.td %x"
			bind $f.td <ButtonRelease-3> \
				"reg_val_change $name -1 $f.td %x"
		}

		set ns $RG($name,NF)
	}

	if $ns {

		# fields present; this implies single value

		frame $f.s -borderwidth 1
		set f $f.s
		pack $f -side left

		# store the tag of the field widget frame
		set RG($name,FW) $f

		for { set i 0 } { $i < $ns } { incr i } {
			set ix "$name,F$i"
			set nm $RG($ix,n)
			set bh $RG($ix,h)
			set bl $RG($ix,l)
			set ti $RG($ix,t)

			append nm "<$bh"
			if { $bh != $bl } {
				append nm ":$bl"
			}
			append nm ">"
			label $f.l$i -text "  $nm" -anchor w
			pack $f.l$i -side left
			if { $ti != "" } {
				tip_set $f.l$i $ti
			}

			# the number of bits in the field
			set nb [expr $bh - $bl + 1]

			while 1 {
				label $f.b$bh \
					-textvariable RG($name,$bh) \
					-bg $lc -font $FO(REG) \
					-cursor $WN(CCD)
				pack $f.b$bh -side left
				if !$ro {
					bind $f.b$bh <ButtonRelease-1> \
						"reg_toggle_bit $name $bh"
					bind $f.b$bh <ButtonRelease-2> \
						"reg_toggle_bit $name $bh"
					bind $f.b$bh <ButtonRelease-3> \
						"reg_toggle_bit $name $bh"
				}
				if { $bh == $bl } {
					break

				}
				incr bh -1
			}
		}
	}

	# the read button
	set f $w.rw$row
	frame $f
	grid $f -column 2 -row $row -sticky news

	label $f.r -text r -cursor $WN(CCD) -bg $WN(RBC)
	pack $f.r -side left -expand y -fill x
	bind $f.r <ButtonRelease-1> "reg_read_click $name"
	if !$ro {
		label $f.w -text w -cursor $WN(CCD) -bg $WN(WBC)
		pack $f.w -side left -expand y -fill x
		bind $f.w <ButtonRelease-1> "reg_write_click $name"
	}
}

proc regset { name val { i "" } } {
#
# Set the register
#
	global RG

	# make sure there are no surprises
	set len $RG($name,L)

	if $len {
		if { $i == "" } {
			# val is a list
			for { set i 0 } { $i < $len } { incr i } {
				set eva [format %02X \
					[expr [lindex $val $i] & 0xFF]]
				set RG($name,V,$i) $eva
				set RG($name,D0,$i) [string index $eva 0]
				set RG($name,D1,$i) [string index $eva 1]
			}
		} else {
			# val is a single value
			set eva [format %02X [expr $val & 0xFF]]
			set RG($name,V,$i) $eva
			set RG($name,D0,$i) [string index $eva 0]
			set RG($name,D1,$i) [string index $eva 1]
		}
		return
	}

	set val [expr $val & 0xFF]
	set eva [format %02X $val]
	set RG($name,V) $eva
	set RG($name,D0) [string index $eva 0]
	set RG($name,D1) [string index $eva 1]

	for { set j 0 } { $j < 8 } { incr j } {
		set RG($name,$j) [expr ($val >> $j) & 1]
	}
}

proc reguse { grp } {
#
# Assume the specified register set
#
	global RGroups RG WN

	foreach p $RGroups($grp) {

		set rg [lindex $p 0]
		set rv [lindex $p 1]

		if ![info exists RG($rg,A)] {
			continue
		}
		if [catch { regset $rg $rv } err] {
			# trc "regset error <$p $rv>: $err"
		}
	}

	if { [string index $grp 0] == "+" } {
		set WN(CRG) "Built In"
	} else {
		set WN(CRG) $grp
	}

	if { $WN(REG) != "" } {
		wm title $WN(REG) "Registers: $WN(CRG)"
	}
}

proc reg_val_change { name dir w x { i 0 } } {
#
# Register value change (from widget)
#
	global RG

	# determine the digit
	set d [winfo width $w]

	if { $x < [expr $d / 2] } {
		set d 0
	} else {
		set d 1
	}

	if $RG($name,L) {
		set ix "$name,D$d,$i"
	} else {
		set ix "$name,D$d"
	}

	set v [expr 0x$RG($ix)]

	if { $dir > 0 } {
		if { $v == 15 } {
			set v 0
		} else {
			incr v
		}
	} else {
		if { $v == 0 } {
			set v 15
		} else {
			incr v -1
		}
	}

	set RG($ix) [format %X $v]

	if $RG($name,L) {
		set v [expr 0x$RG($name,D0,$i)$RG($name,D1,$i)]
		regset $name $v $i
		return
	}

	set v [expr 0x$RG($name,D0)$RG($name,D1)]
	regset $name $v
}

proc reg_toggle_bit { name i } {
#
# Toggles the indicated bit in the register
#
	global RG

	# single-valued registers only
	set val [regval $name]
	set bit [expr 1 << $i]

	if [expr $val & $bit] {
		set val [expr $val ^ $bit]
	} else {
		set val [expr $val | $bit]
	}

	regset $name $val
}

proc reg_write { name } {

	global ST RG PM

	if { $ST(SFD) == "" } {
		# disconnected, this probably will never happen
		return "Cannot write register, not connected to device"
	}

	if ![info exists RG($name,A)] {
		# impossible
		return "Cannot write register $name, no such register"
	}

	if $RG($name,S) {
		return "Cannot write register $name, register is read-only"
	}

	set val [regval $name]
	set seq [rqc]

	if $RG($name,L) {
		# burst
		# trc "RWB: $name, $RG($name,A), $RG($name,L) == $val"
		set cmd [binary format cccc 0x06 $seq $RG($name,A) $RG($name,L)]
		foreach v $val {
			append cmd [pack1 $v]
		}
	} else {
		# single
		# trc "RWS: $name, $RG($name,A) == $val"
		set cmd [binary format ccccc 0x03 $seq 1 $RG($name,A) $val]
	}

	for { set i 0 } { $i < $PM(RET) } { incr i } {
		noss_send $cmd
		if { [wack 0xFD $seq 2000] >= 0 } {
			return ""
		}
	}

	return "Failed to write register $name, node response timeout"
}

proc reg_write_click { name } {

	set err [reg_write $name]

	if { $err != "" } {
		alert $err
	}
}

proc regs_write { } {
#
# Writes all R/W registers to the device
#
	global ST RG PM RGroups

	if { $ST(SFD) == "" } {
		# disconnected
		return "Cannot write registers, not connected to device"
	}

	# single set
	set gs ""
	# multiple set
	set gm ""

	foreach r $RGroups(+RW) {
		set r [lindex $r 0]
		if $RG($r,L) {
			# multivalue
			lappend gm $r
		} else {
			lappend gs [list $RG($r,A) [regval $r]]
		}
	}

	set seq [rqc]
	set cmd [binary format ccc 0x03 $seq [llength $gs]]
	foreach r $gs {
		append cmd [pack1 [lindex $r 0]]
		append cmd [pack1 [lindex $r 1]]
	}

	# write single-valued registers
	for { set i 0 } { $i < $PM(RET) } { incr i } {
		noss_send $cmd
		if { [wack 0xFD $seq 3000] >= 0 } {
			break
		}
	}

	if { $i == $PM(RET) } {
		return "Failed to write single-valued registers, node response\
			timeout"
	}

	# take care of the burst registers; there's probably just one, i.e.,
	# the PATABLE
	foreach r $gm {
		set err [reg_write $r]
		if { $err != "" } {
			return $err
		}
	}

	return ""
}

proc regs_write_click { } {

	set err [regs_write]

	if { $err != "" } {
		alert $err
	}
}
	
proc reg_read { name } {
#
# Read register from device
#
	global ST RG PM

	if { $ST(SFD) == "" } {
		return "Cannot read register $name, not connected to device"
	}

	if ![info exists RG($name,A)] {
		return "Cannot read register $name, no such register"
	}

	set seq [rqc]

	if $RG($name,L) {
		# burst
		set wai 0x07
		set cmd [binary format cccc $wai $seq $RG($name,A) $RG($name,L)]
	} else {
		# single
		set wai 0x04
		set cmd [binary format cccc $wai $seq 1 $RG($name,A)]
	}

	for { set i 0 } { $i < $PM(RET) } { incr i } {
		noss_send $cmd
		if { [wack $wai $seq 2000] >= 0 } {
			break
		}
	}

	if { $i == $PM(RET) } {
		return "Failed to read register $name, node response timeout"
	}

	# the register has been read
	set rep $ST(ECM,M)
	set ST(ECM,M) ""
	set al [string length $rep]

	if $RG($name,L) {
		# burst
		set el [expr $RG($name,L) + 1]
		if { $al < $el } {
			return "Failed to read register $name, response too\
				short, expected $el, got $al bytes"
		}
		incr el -1
		binary scan [string index $rep 0] cu al
		if { $al != $el } {
			return "Failed to read register $name, expected $el\
				bytes, got $al"
		}
		binary scan [string range $rep 1 end] cu${al} vl
	} else {
		# single value
		if { $al < 2 } {
			return "Failed to read register $name, response too\
				short"
		}
		binary scan $rep cucu el vl
		if { $el != 1 } {
			return "Failed to read register $name, illegal\
				response from node"
		}
	}
		
	regset $name $vl

	return ""
}

proc reg_read_click { name } {

	set err [reg_read $name]

	if { $err != "" } {
		alert $err
	}
}

proc regs_read { } {
#
# Read all registers
#
	global ST RG PM RGroups

	if { $ST(SFD) == "" } {
		# disconnected
		return "Cannot read registers, not connected to device"
	}

	set gs ""
	set gm ""
	foreach r $RGroups(+RW) {
		set r [lindex $r 0]
		if $RG($r,L) {
			lappend gm $r
		} else {
			lappend gs $r
		}
	}

	if 0 {
		# R/O registers can only be read individually
		foreach r $RGroups(+RO) {
			if $RG($r,L) {
				lappend gm $r
			} else {
				lappend gs $r
			}
		}
	}

	# start with the burst ones
	foreach r $gm {
		set err [reg_read $r]
		if { $err != "" } {
			return $err
		}
	}


	# now for the single-valued ones

	set el [llength $gs]
	set seq [rqc]
	set cmd [binary format ccc 0x04 $seq $el]

	foreach r $gs {
		append cmd [pack1 $RG($r,A)]
	}

	# read all single-valued registers
	for { set i 0 } { $i < $PM(RET) } { incr i } {
		noss_send $cmd
		if { [wack 0x04 $seq 2000] >= 0 } {
			break
		}
	}

	if { $i == $PM(RET) } {
		return "Failed to read registers, node response timeout"
	}

	set rep $ST(ECM,M)
	set ST(ECM,M) ""
	set al [string length $rep]

	if { $al < [expr $el + 1] } {
		return "Failed to read registers, node response too short,\
			expected [expr $el + 1] bytes, got $al"
	}

	binary scan [string index $rep 0] cu al

	if { $al != $el } {
		return "Failed to read registers, got back only $al values,\
			expected $el"
	}

	binary scan [string range $rep 1 end] cu${al} vl

	foreach r $gs v $vl {
		regset $r $v
	}


	return ""
}

proc regs_read_click { } {

	set err [regs_read]

	if { $err != "" } {
		alert $err
	}
}

proc regs_toggle { } {
#
# Open/close the registers window
#
	global WN

	if { $WN(REG) == "" } {
		regs_open
	} else {
		regs_close
	}
}

proc regs_close { } {

	global WN

	catch { destroy $WN(REG) }
	set WN(REG) ""
	array unset WN "REG,*"
	$WN(REB) configure -text "Show Regs"
}

proc regs_open { } {
#
# Opens the registers window
#
	global WN RGroups

	if { $WN(REG) != "" } {
		# already open
		return
	}

	$WN(REB) configure -text "Hide Regs"

	set w ".regw"
	toplevel $w
	wm title $w "Registers: $WN(CRG)"

	set WN(REG) $w
	set WN(REG,CON) ""

	bind $w <Destroy> regs_close

	set f $w.mf
	frame $f
	pack $f -side left -expand y -fill both

	set b $w.bf
	frame $b
	pack $b -side left -expand n -fill y

	#######################################################################
	# buttons #############################################################
	#######################################################################
	button $b.c -text Close -command regs_close
	pack $b.c -side bottom -expand n -fill x

	button $b.w -text Write -command regs_write_click
	pack $b.w -side bottom -expand n -fill x
	lappend WN(REG,CON) $b.w

	button $b.r -text Read -command regs_read_click
	pack $b.r -side bottom -expand n -fill x
	lappend WN(REG,CON) $b.r

	button $b.l -text Load -command regs_load
	pack $b.l -side top -expand n -fill x

	button $b.s -text Save -command regs_save
	pack $b.s -side top -expand n -fill x

	#######################################################################
	#######################################################################

	ttk::frame $f.tf
	set f $f.tf
	pack $f -expand y -fill both


	scrollbar $f.sb -orient vertical -command "$f.tt yview" -takefocus 1
	scrollbar $f.sc -orient horizontal -command "$f.tt xview" -takefocus 1
	pack $f.sb -side right -fill y
	pack $f.sc -side bottom -fill x

	text $f.tt -yscrollcommand "$f.sb set" -xscrollcommand "$f.sc set" \
		-cursor $WN(CRP) -width 80 -state disabled
	pack $f.tt -expand y -fill both -padx 1

	set x $f.tt.r
	frame $x

	# more ...
	set row 0

	foreach r $RGroups(+RW) {
		regwid [lindex $r 0] $x $row
		incr row
	}

	foreach r $RGroups(+RO) {
		regwid $r $x $row
		incr row
	}

	$f.tt window create end -window $x

	update_widgets
}

proc regs_load { } {
#
# Load a new register set
#
	global WN RGroups Mod

	# the list of groups
	set gl ""

	foreach g [array names RGroups] {
		if { [string index $g 0] != "+" } {
			lappend gl $g
		}
	}

	set gl [concat [list "Built In"] [lsort $gl]]

	if { $gl == "" } {
		alert "No saved register sets are available"
		return
	}

	# we need a simple modal window
	set w [md_window "Choose register set"]
	set f $w
	set Mod(0,SE) [lindex $gl 0]

	eval "tk_optionMenu $f.op Mod(0,SE) $gl"
	pack $f.op -side top -expand y -fill x

	set f $w.bf
	frame $f
	pack $f -side top -expand y -fill x

	button $f.c -text Cancel -anchor w -command "md_click -1"
	pack $f.c -side left

	button $f.l -text Load -anchor e -command "md_click 1"
	pack $f.l -side right

	button $f.d -text Delete -anchor e -command "md_click 2"
	pack $f.d -side right

	bind $w <Destroy> "md_click -1"

	#######################################################################

	while 1 {

		set ev [md_wait]

		if { $ev < 0 } {
			return
		}

		set v [string trim $Mod(0,SE)]
		if { $v == "" || [string index $v 0] == "+" } {
			# a precaution
			continue
		}

		if { $ev == 1 } {
			reguse "+RW"
			if { $v != "Built In" } {
				reguse $v
			}
			md_stop
			return
		}

		if { $ev == 2 } {
			if { $v == "Built In" } {
				alert "This register set cannot be removed"
				continue
			}
			if [info exists RGroups($v)] {
				if [confirm "Do you really want to remove the\
				    register set: $v"] {
					unset RGroups($v)
					md_stop
					return
				}
			}
		}
	}
}

proc regs_save { } {
#
# Save the current register set
#
	global WN RGroups Mod

	# the list of groups
	set gl ""

	foreach g [array names RGroups] {
		if { [string index $g 0] != "+" } {
			lappend gl $g
		}
	}

	set gl [lsort $gl]

	set w [md_window "Save register set"]
	set f $w.t

	frame $f
	pack $f -side top -expand y -fill x

	label $f.l -text "Name to save under:" -anchor w
	pack $f.l -side top -expand n -anchor w


	set Mod(0,SE) [ttk::combobox $f.op -values $gl]
	pack $f.op -side top -expand y -fill x

	set f $w.bf
	frame $f
	pack $f -side top -expand y -fill x

	button $f.c -text Cancel -anchor w -command "md_click -1"
	pack $f.c -side left

	button $f.l -text Save -anchor e -command "md_click 1"
	pack $f.l -side right

	bind $w <Destroy> "md_click -1"

	#######################################################################

	while 1 {

		set ev [md_wait]

		if { $ev < 0 } {
			return
		}

		if { $ev == 1 } {
			set n [string trim [$Mod(0,SE) get]]
			if { $n == "Built In" } {
				alert "This register set cannot be overwritten"
				continue
			}
			if { $n == "" || [string index $n 0] == "+" } {
				alert "This name: $n is illegal"
				continue
			}
			set gr ""
			foreach r $RGroups(+RW) {
				set r [lindex $r 0]
				lappend gr [list $r [regval $r]]
			}
			set RGroups($n) $gr
			md_stop
			return
		}
	}
}

###############################################################################

proc unique { } {
#
# Returns a unique ID
#
	global WN

	incr WN(UNQ)
	return "u$WN(UNQ)"
}

proc inject_wl { } {
#
# Returns the list of "this" tags for all inject windows currently present
#
	global WN

	set wl [array names WN "INJ,*,WN"]
	set re ""
	foreach w $wl {
		set r ""
		regexp ",(.+)," $w j r
		if { $r != "" } {
			lappend re $r
		}
	}
	return $re
}

proc inject_update_db_selection { } {
#
# Called whenever the layout selection changes to update the selection menus
# at all packet injection windows
#
	global WN PLayouts

	if { [array names PLayouts] != "" } {
		set s "normal"
	} else {
		set s "disabled"
	}

	foreach t [inject_wl] {

		if ![info exists WN(INJ,$t,WN)] {
			# a precaution
			continue
		}

		# Load/Delete
		$WN(INJ,$t,BL) configure -state $s
	}
}

proc inject_update_save_buttons { this } {
#
# Called whenever the status of either of the two save buttons has to be
# updated
#
	global WN

	if ![info exists WN(INJ,$this,BS)] {
		# the usual precaution
		return
	}

	set sa "disabled"
	set sb $sa

	if { $WN(INJ,$this,LA) != "" } {
		set sa "normal"
		if { $WN(INJ,$this,NM) != "" } {
			set sb "normal"
		}
	}

	$WN(INJ,$this,BS) configure -state $sb
	$WN(INJ,$this,BA) configure -state $sa
}

proc inject_button_status { } {
#
# Activate/deactivate the Inject/Repeat buttons
#
	global WN ST

	foreach t [inject_wl] {
		set w0 $WN(INJ,$t,BI)
		set w1 $WN(INJ,$t,BR)
		set s0 "disabled"
		set s1 "disabled"
		if { $ST(SIP) >= 2 && $WN(INJ,$t,LA) != "" } {
			if { $WN(INJ,$t,CB) == "" } {
				set s0 "normal"
			}
			set rp [lindex [lindex $WN(INJ,$t,LA) 0] 1]
			if $rp {
				set s1 "normal"
			}
		} else {
			if { $WN(INJ,$t,CB) != "" } {
				catch { after cancel $WN(INJ,$t,CB) }
				set WN(INJ,$t,CB) ""
			}
		}
		$w0 configure -state $s0
		if { $WN(INJ,$t,CB) == "" } {
			set tx "Repeat"
		} else {
			set tx "Stop Repeat"
		}
		$w1 configure -state $s1 -text $tx
	}
}

proc inject_parse_hex_list { val } {
#
# Parses a list of hex pairs returning the list of values
#
	set vl ""
	while 1 {
		set val [string trimleft $val]
		if { $val == "" } {
			return $vl
		}
		set c ""
		regexp "^(\[^ \t\]+)" $val c
		set l [string length $c]
		if { $l > 2 } {
			# failed
			return ""
		}
		if [catch { expr 0x$c } c] {
			# not hex
			return ""
		}
		lappend vl $c
		incr l
		set val [string range $val $l end]
	}
}

proc inject_value_to_bytes { val typ siz } {
#
# Converts a value of the given type to a string of bytes
#
	global CH
	upvar $siz size

	set typ [string index $typ 0]
	set val [string trim $val]
	# this can be h, o, i; only h admits spaces separating pairs of
	# digits
	if { $typ == "h" } {
		if { $val == "" } {
			set val 0
		}
		# decode the values into a list
		set vals [inject_parse_hex_list $val]
		if { $vals == "" } {
			# try a single value
			if [catch { expr 0x$val } vals] {
				# no way
				error "illegal hex value (or list thereof)"
			}
		}
		set ll [llength $vals]
		if { $size == "-" } {
			if { $ll > 1 } {
				# this is the size, one byte per value
				set size $ll
			} else {
				# otherwise (single item), the size is
				# determined by the range
				set size [intsize [lindex $vals 0]]
			}
		}
		# pack the bytes
		if { $ll > 1 } {
			if { $ll > $size } {
				# make this an error
				error "hex list longer than requested size"
			}
			set bb ""
			for { set i 0 } { $i < $size } { incr i } {
				set v [lindex $vals $i]
				if { $v == "" } {
					set v 0
				}
				append bb [binary format c $v]
			}
		} else {
			# a single value
			set bb [int_to_bytes [lindex $vals 0] $size]
		}
		return $bb
	}

	if { $typ == "o" || $typ == "i" } {
		if { $val == "" } {
			set val 0
		}
		if { $typ == "o" && [string index $val 0] != 0 } {
			set val "0$val"
		}
		if [catch { expr $val } $val] {
			error "illegal $typ value"
		}
		if { $size == "-" } {
			set size [intsize $val]
		}
		return [int_to_bytes $val $size]
	}

	# we have a string
	if { [string index $val 0] != "\"" } {
		set val "\"$val\""
	}

	if [catch { stparse val } vals] {
		error $vals
	}

	set nc [llength $vals]
	if { $nc == 0 || [lindex $vals end] != 0 } {
		# force a sentinel
		lappend vals 0
		incr nc
	}
	if { $size == "-" } {
		# implied by the string length
		set size $nc
	}

	if { $size < [expr $nc - 1] } {
		# consider the sentinel optional
		error "size insufficient to accommodate string"
	}

	set bb ""
	set i 0
	foreach b $vals {
		if { $i == $size } {
			break
		}
		append bb [binary format c $b]
		incr i
	}
	while { $i < $size } {
		append bb $CH(ZER)
		incr i
	}
	return $bb
}

proc inject_incr_value { val typ how } {
#
# Increment the value
#
	if { $typ == "hex" } {
		set v 0x$val
	} elseif { $typ == "oct" && [string index $val 0] != 0 } {
		set v 0$val
	} else {
		set v $val
	}

	set v [expr { $v + $how }]

	# format it properly
	if { $typ == "hex" } {
		set v [format %02X $v]
	} elseif { $typ == "oct" } {
		set v [format %03o $v]
	}
	return $v
}

proc inject_update_layout { this } {
#
# Updates the packet layout based on the current content of widgets
#
	global WN PM CH

	if ![info exists WN(INJ,$this,WN)] {
		# in case the window has disappeared in the meantime
		return ""
	}

	# packet length
	set ln 0

	# layout
	set WN(INJ,$this,LA) ""
	set WN(INJ,$this,PL) 0
	set la ""

	# disable buttons: Inject + Repeat
	inject_button_status
	# disable them as well
	inject_update_save_buttons $this

	# clean the content labels and byte numbers
	for { set i 0 } { $i < $WN(NLE) } { incr i } {
		set WN(INJ,$this,N$i) "-"
		set WN(INJ,$this,B$i) [string repeat " " $WN(NCC)]
	}

	for { set i 0 } { $i < $WN(NLE) } { incr i } {

		# component number
		set ci [expr $i + 1]

		set sz $WN(INJ,$this,S$i)

		set val [string trim $WN(INJ,$this,V$i)]

		if { $sz == "-" && $val == "" } {
			# append a null element to keep the list length
			# consistent with the numbering of fields
			lappend la ""
			continue
		}

		# the type
		set ty $WN(INJ,$this,T$i)

		# the size + some obsessive precaution
		if { $sz != "-" } {
			# to be determined from the value
			if [catch { expr $sz } sz] {
				# a precaution, it can only be a number now
				return "Illegal size for component $ci, must be\
					an int <= $PM(MPS), -, or ?"
			}
		}

		# turn into bytes
		if [catch { inject_value_to_bytes $val $ty sz } bb] {
			return "Component $ci: $bb"
		}

		set WN(INJ,$this,N$i) $ln

		set npl [expr $sz + $ln]

		if { $npl > $PM(MPS) } {
			return "The size of component $ci ($sz) yields total\
				packet length of $npl, which exceeds the max\
				of $PM(MPS) by [expr $npl - $PM(MPS)]"
		}

		set in $WN(INJ,$this,I$i)
		if [catch { expr $in } in] {
			set in 0
		}

		set WN(INJ,$this,S$i) $sz

		if $in {
			# increment/decrement test
			if [catch { inject_incr_value $val $ty 1 } ] {
				return "Component $ci: the value cannot be\
					incremented or decremented"
			}
		}

		lappend la [list $ln $sz $ty $val $in $bb]

		set ln $npl
	}

	if { $ln == 0 } {
		return "No components, zero packet length"
	}

	if $WN(INJ,$this,SC) {
		# software CRC
		if [expr $ln & 1] {
			return "Software CRC can only be computed when packet\
				length is even (it is $ln)"
		}
		set mcpl [expr ($PM(MPS) - 2) & ~1]
		if { $ln > $mcpl } {
			return "For software CRC, the packet must not be longer\
				than $mcpl bytes"
		}
	}

	# Repeat/Delay
	if [catch { inject_get_repeat_params $this } rp] {
		return $rp
	}

	# fill the contents
	set last $i

	for { set i 0 } { $i < $last } { incr i } {
		set WN(INJ,$this,B$i) [inject_bytes_to_contents \
			[lindex [lindex $la $i] 5]]
	}

	# insert the three special parameters in front
	set WN(INJ,$this,LA) [linsert $la 0 \
		[list $WN(INJ,$this,SC) [lindex $rp 0] [lindex $rp 1]]]

	set WN(INJ,$this,PL) $ln

	inject_update_save_buttons $this
	inject_button_status

	return ""
}

proc inject_update_click { this } {

	set err [inject_update_layout $this]

	if { $err != "" } {
		alert $err
	}
}

proc inject_get_repeat_params { this } {
#
# Two parameters of the repeat mode: count and delay; throws exceptions
#
	global WN

	if ![info exists WN(INJ,$this,RC)] {
		return [list 0 0]
	}

	set rc [string trim $WN(INJ,$this,RC)]
	set de [string trim $WN(INJ,$this,DE)]

	if { $rc == "" || $rc == 0 } {
		if { $de != "" && $de != 0 } {
			error "Repeat count is empty while Delay isn't"
		}
		return [list 0 0]
	}

	if { ![isinteger $rc] || $rc < 0 } {
		error "Repeat count must be an integer >= 0"
	}

	if { ![isinteger $de] || $de <= 0 } {
		error "Delay must be an integer > 0"
	}

	return [list $rc $de]
}

proc inject_bytes_to_contents { bb } {
#
# Converts raw bytes to the contents label
#
	global WN

	set ll [string length $bb]

	if { $ll > $WN(NCM) } {
		set ll [expr $WN(NCM) - 1]
		set ap "..."
	} else {
		set ap ""
	}

	set lb ""

	for { set i 0 } { $i < $ll } { incr i } {
		set c [string index $bb $i]
		binary scan $c cu v
		if { $lb != "" } {
			append lb " "
		}
		append lb [format %02X $v]
	}

	append lb $ap

	return $lb
}

proc inject_save { this how } {
#
# Saves the current packet layout
#
	global WN PLayouts

	if { ![info exists WN(INJ,$this,WN)] || $WN(INJ,$this,LA) == "" } {
		alert "No layout to save"
		return
	}

	if { $how == 0 && $WN(INJ,$this,NM) == "" } {
		# force "Save As" is name unknown
		set how 1
	}

	if $how {
		# we need a dialog
		set ll [lsort [array names PLayouts]]
		set w [md_window "Save layout"]
		set f $w.t
		frame $f
		pack $f -side top -expand y -fill x

		label $f.l -text "Name to save under:" -anchor w
		pack $f.l -side top -expand n -anchor w

		set Mod(0,SE) [ttk::combobox $f.op -values $ll]
		pack $f.op -side top -expand y -fill x

		set f $w.bf
		frame $f
		pack $f -side top -expand y -fill x

		button $f.c -text Cancel -anchor w -command "md_click -1"
		pack $f.c -side left

		button $f.l -text Save -anchor e -command "md_click 1"
		pack $f.l -side right

		bind $w <Destroy> "md_click -1"

		###############################################################

		while 1 {

			set ev [md_wait]

			if { $ev < 0 } {
				return
			}

			if { $ev == 1 } {
				set name [string trim [$Mod(0,SE) get]]
				if { $name != "" } {
					md_stop
					break
				}
				alert "Name to save under cannot be empty"
			}
		}
	} else {
		set name $WN(INJ,$this,NM)
	}

	set la ""

	foreach it $WN(INJ,$this,LA) {
		if { [llength $it] >= 5 } {
			lappend la [list [lindex $it 1] [lindex $it 2] \
				[lindex $it 3] [lindex $it 4]]
		} else {
			lappend la $it
		}
	}

	set PLayouts($name) $la

	inject_update_db_selection
}

proc inject_load { this } {
#
# Load the current DB selection into the layout
#
	global WN PLayouts PM Mod

	# the list of layouts
	set ll [lsort [array names PLayouts]]
	if { $ll == "" } {
		alert "No layouts are available"
		return
	}

	# we need a simple modal window
	set w [md_window "Select layout to load"]
	set f $w
	set Mod(0,SE) [lindex $ll 0]

	eval "tk_optionMenu $f.op Mod(0,SE) $ll"
	pack $f.op -side top -expand y -fill x

	set f $w.bf
	frame $f
	pack $f -side top -expand y -fill x

	button $f.c -text Cancel -anchor w -command "md_click -1"
	pack $f.c -side left

	button $f.l -text Load -anchor e -command "md_click 1"
	pack $f.l -side right

	button $f.d -text Delete -anchor e -command "md_click 2"
	pack $f.d -side right

	bind $w <Destroy> "md_click -1"

	#######################################################################

	while 1 {

		set ev [md_wait]

		if { $ev < 0 } {
			return
		}

		set name [string trim $Mod(0,SE)]
		if { $name == "" } {
			# a precaution
			continue
		}

		if { $ev == 1 } {
			# Load
			md_stop
			break
		}

		if { $ev == 2 } {
			# Delete
			if [info exists PLayouts($name)] {
				if [confirm "Do you really want to remove the\
				    layout: $name"] {
					unset PLayouts($name)
					md_stop
					inject_update_db_selection
					return
				}
			}
		}
	}

	if ![info exists PLayouts($name)] {
		alert "The layout $name doesn't exist in database"
		return
	}

	inject_reset $this

	set n [llength $PLayouts($name)]
	set e "The DB layout appears to be broken"
	if { $n < 2 } {
		alert $e
		return
	}

	incr n -1
	if { $n > $WN(NLE) } {
		set n $WN(NLE)
	}

	set it [lindex $PLayouts($name) 0]
	if { [llength $it] != 3 } {
		alert $e
		return
	}

	foreach i $it {
		if ![isinteger $i] {
			alert $e
			return
		}
	}

	if [lindex $it 0] {
		set WN(INJ,$this,SC) 
	}

	set WN(INJ,$this,RC) [lindex $it 1]
	set WN(INJ,$this,DE) [lindex $it 2]

	set la [lrange $PLayouts($name) 1 end]

	for { set i 0 } { $i < $n } { incr i } {

		set it [lindex $la $i]

		if { [llength $it] != 4 } {
			continue
		}

		# size, type, value, increment
		lassign $it sz ty va in

		if { [lsearch -exact $PM(LAS) $sz] < 0 } {
			set sz "?"
		}

		if { [lsearch -exact $PM(VTY) $ty] < 0 } {
			set ty [lindex $PM(VTY) 0]
		}

		set WN(INJ,$this,S$i) $sz
		set WN(INJ,$this,T$i) $ty

		if [catch { expr $in } in] {
			set in 0
		}

		if { $in < 0 } {
			set in "-1"
		} elseif { $in > 0 } {
			set in "+1"
		} else {
			set in "0"
		}
		set WN(INJ,$this,I$i) $in
		set WN(INJ,$this,V$i) $va
	}

	inject_setname $this $name
	inject_update_layout $this
}

proc inject_close { this } {
#
	global WN

	if ![info exists WN(INJ,$this,WN)] {
		return
	}

	if { $WN(INJ,$this,CB) != "" } {
		catch { after cancel $WN(INJ,$this,CB) }
	}

	catch { destroy $WN(INJ,$this,WN) }

	array unset WN "INJ,$this,*"
}

proc inject_reset { this } {
#
# Zeroes out the current layout
#
	global WN

	if ![info exists WN(INJ,$this,WN)] {
		return
	}

	set WN(INJ,$this,LA) ""
	set WN(INJ,$this,PL) 0
	inject_setname $this ""
	set WN(INJ,$this,DE) ""
	set WN(INJ,$this,RC) ""
	set WN(INJ,$this,SC) 0

	for { set i 0 } { $i < $WN(NLE) } { incr i } {
		set WN(INJ,$this,N$i) "-"
		set WN(INJ,$this,T$i) "hex"
		set WN(INJ,$this,S$i) "-"
		set WN(INJ,$this,V$i) ""
		set WN(INJ,$this,I$i) "0"
	}

	inject_update_save_buttons $this
	inject_update_layout $this
}

proc inject_setname { this name } {
#
# Sets the name attribute of an inject window
#
	global WN

	if ![info exists WN(INJ,$this,WN)] {
		return
	}

	set WN(INJ,$this,NM) $name

	if { $name == "" } {
		set ap ""
	} else {
		set ap " (layout $name)"
	}
	
	wm title $WN(INJ,$this,WN) "Packet to inject$ap"
}

proc inject_click { } {
#
# Opens a packet injection window
#
	global WN PM FO

	set this [unique]
	set w ".i$this"

	toplevel $w
	wm title $w "Packet to inject"

	# this is for the record; we could easily recover the window path from
	# "this" (see above), but we will also use this entry to tell whether
	# the window for the given tag is present at all (i.e. when the entry
	# exists)
	set WN(INJ,$this,WN) $w

	# callback for repeat injection
	set WN(INJ,$this,CB) ""

	set tf $w.f
	frame $tf
	pack $tf -side top -fill x -expand n

	#######################################################################

	# start with an empty layout
	set WN(INJ,$this,LA) ""
	# packet length
	set WN(INJ,$this,PL) 0
	# it also happens to be unnamed
	inject_setname $this ""
	# Delay/Repeat count
	set WN(INJ,$this,DE) ""
	set WN(INJ,$this,RC) ""
	# no software CRC
	set WN(INJ,$this,SC) 0

	#######################################################################

	set f $tf.ls
	frame $f
	pack $f -side left -expand n -fill x -anchor w

	# Reset, Save, Save As
	set x $f.rb
	button $x -text Reset -command "inject_reset $this"
	pack $x -side left -expand n -anchor w
	# Reset is always enabled, it doesn't hurt

	set x $f.lb
	button $x -text Load/Delete -command "inject_load $this"
	pack $x -side left -expand n -anchor w
	set WN(INJ,$this,BL) $x

	set x $f.sb
	button $x -text Save -command "inject_save $this 0"
	pack $x -side left -expand n -anchor w
	# save is enabled if layout nonempty and named
	set WN(INJ,$this,BS) $x

	set x $f.ab
	button $x -text "Save As" -command "inject_save $this 1"
	pack $x -side left -expand n -anchor w
	# save as is enabled if layout nonempty
	set WN(INJ,$this,BA) $x

	set x $f.vb
	button $x -text Verify/Update -command "inject_update_click $this"
	pack $x -side left -expand n

	#######################################################################

	# layout widgets

	set f $w.w
	frame $f
	pack $f -side top -fill x -expand y

	# the legend
	label $f.n -text Byte -justify left -anchor w
	grid $f.n -column 0 -row 0 -sticky w -padx 1 -pady 4

	label $f.s -text Size -justify left -anchor w
	grid $f.s -column 1 -row 0 -sticky w -padx 1 -pady 4

	label $f.t -text Type -justify left -anchor w
	grid $f.t -column 2 -row 0 -sticky w -padx 1 -pady 4

	label $f.v -text Value -justify left -anchor w
	grid $f.v -column 3 -row 0 -sticky w -padx 1 -pady 4

	label $f.i -text Incr -justify left -anchor w
	grid $f.i -column 4 -row 0 -sticky w -padx 1 -pady 4

	label $f.b -text Content -justify left -anchor w
	grid $f.b -column 5 -row 0 -sticky w -padx 1 -pady 4

	set row 1
	for { set i 0 } { $i < $WN(NLE) } { incr i } {

		# byte number
		set WN(INJ,$this,N$i) "-"
		label $f.n$i -textvariable WN(INJ,$this,N$i) -justify right \
			-anchor e -font $FO(INJ)
		grid $f.n$i -column 0 -row $row -sticky w -padx 1 -pady 1

		# size selection
		set WN(INJ,$this,S$i) "-"
		spinbox $f.s$i -textvariable WN(INJ,$this,S$i) \
			-values $PM(LAS) -wrap 1 -width 2 -state readonly
		grid $f.s$i -column 1 -row $row -sticky w -padx 1 -pady 1

		# value type
		set WN(INJ,$this,T$i) [lindex $PM(VTY) 0]
		spinbox $f.t$i -textvariable WN(INJ,$this,T$i) \
			-values $PM(VTY) \
			-wrap 1 -width 3 -state readonly
		grid $f.t$i -column 2 -row $row -sticky w -padx 1 -pady 1

		# value entry box; this column will be expandable
		set WN(INJ,$this,V$i) ""
		entry $f.v$i -width 8 -font $FO(INJ) \
			-textvariable WN(INJ,$this,V$i)
		grid $f.v$i -column 3 -row $row -sticky ew -padx 1 -pady 1
		bind $f.v$i <ButtonRelease-1> "tk_textCopy $f.v$i"
		#bind $f.v$i <ButtonRelease-2> "tk_textPaste $f.v$i"

		# increment choice
		set WN(INJ,$this,I$i) "0"
		tk_optionMenu $f.i$i WN(INJ,$this,I$i) "-1" "0" "+1"
		grid $f.i$i -column 4 -row $row -sticky w -padx 1 -pady 1

		# content
		set WN(INJ,$this,B$i) [string repeat " " $WN(NCC)]
		label $f.b$i -textvariable WN(INJ,$this,B$i) -justify left \
			-anchor w -font $FO(INJ)
		grid $f.b$i -column 5 -row $row -sticky w -padx 1 -pady 1

		incr row
	}

	grid columnconfigure $f { 0 1 2 4 5 } -weight 0
	grid columnconfigure $f 3 -weight 1

	#######################################################################

	set f $w.a
	frame $f
	pack $f -side top -expand y -fill x

	label $f.lh -text "Packet length: "
	pack $f.lh -side left -expand n

	label $f.ll -textvariable WN(INJ,$this,PL) -bg gray
	pack $f.ll -side left -expand n

	checkbutton $f.sc -variable WN(INJ,$this,SC)
	pack $f.sc -side right -expand n

	label $f.sl -text "  Soft CRC:"
	pack $f.sl -side right -expand n

	entry $f.de -width 4 -font $FO(INJ) -textvariable WN(INJ,$this,DE)
	pack $f.de -side right -expand n

	label $f.dl -text "  Delay:"
	pack $f.dl -side right -expand n

	entry $f.rc -width 4 -font $FO(INJ) -textvariable WN(INJ,$this,RC)
	pack $f.rc -side right -expand n

	label $f.rl -text "  Repeat:"
	pack $f.rl -side right -expand n

	#######################################################################

	set f $w.b
	frame $f -relief solid -bd 1
	pack $f -side top -expand y -fill x -anchor s

	button $f.cl -text "Close" -command "inject_close $this"
	pack $f.cl -side left -expand n

	button $f.in -text "Inject" -command "inject_execute $this"
	set WN(INJ,$this,BI) $f.in
	pack $f.in -side right -expand n

	button $f.rp -text "Repeat" -command "inject_repeat $this"
	set WN(INJ,$this,BR) $f.rp
	pack $f.rp -side right -expand n
	
	# initial fill
	inject_update_db_selection
	inject_update_save_buttons $this
	inject_update_layout $this

	bind $w <Destroy> "inject_close $this"
}

proc inject_build_packet { this } {
#
# Build a packet to inject
#
	global WN CH

	if ![info exists WN(INJ,$this,WN)] {
		return ""
	}

	set pkt ""

	# the list of components
	set la $WN(INJ,$this,LA)
	# software checksum flag
	set lf [lindex $la 0]
	set sc [lindex $lf 0]
	# skip the special entry
	set la [lrange $la 1 end]

	set nl [list $lf]
	set i 0

	foreach it $la {
		if { [llength $it] >= 6 } {
			lassign $it bn sz ty va in bb
			if { $bb == "" } {
				# something wrong
				return ""
			}
			append pkt $bb
			if $in {
				# increment/decrement
				if ![catch { inject_incr_value $va $ty $in } \
				    v] {
					set WN(INJ,$this,V$i) $v
					if [catch { inject_value_to_bytes $v \
					    $ty sz } vv] {
						# something went wrong, revert
						set v $va
					} else {
						set bb $vv
					}
					set WN(INJ,$this,B$i) \
						[inject_bytes_to_contents $bb]
					# update the item
					set it [list $bn $sz $ty $v $in $bb]
				}
			}
		}
		lappend nl $it
		incr i
	}

	set WN(INJ,$this,LA) $nl

	if $sc {
		# software checksum
		if [expr [string length $pkt] & 1] {
			append pkt $CH(ZER)
		}
		set pkt "$pkt[noss_chks $pkt]"
	}

	return $pkt
}
				
proc inject_execute { this } {

	global WN PM

	if ![info exists WN(INJ,$this,WN)] {
		return
	}

	if { $WN(INJ,$this,CB) != "" } {
		# repeating, ignore, the button should be disabled anyway
		return
	}

	$WN(INJ,$this,BI) configure -state disabled
	$WN(INJ,$this,BR) configure -state disabled

	set pkt [inject_build_packet $this]

	if { $pkt == "" } {
		alert "Cannot generate a packet from this layout"
		inject_button_status
		return
	} 

	log "** injecting packet \[[tstamp]\] (length=[string length $pkt]):"
	log "=> [string trimleft [hencode $pkt]]"

	set seq [rqc]
	set msg "[binary format ccc 0x08 $seq [string length $pkt]]$pkt"

	for { set i 0 } { $i < $PM(RET) } { incr i } {
		noss_send $msg
		set ret [wack { 0xFD 0xFC } $seq 2000]
		if { $ret == 0xFD } {
			break
		}
	}

	if { $ret < 0 } {
		alert "Node response timeout"
	} elseif { $ret != 0xFD } {
		alert "Request rejected by node"
	}

	inject_button_status
}

proc inject_repeat { this } {

	global WN

	if ![info exists WN(INJ,$this,WN)] {
		return
	}

	if { $WN(INJ,$this,CB) != "" } {
		# repeating, cancel
		catch { after cancel $WN(INJ,$this,CB) }
		set WN(INJ,$this,CB) ""
		inject_button_status
		return
	}

	# start repeat
	set la $WN(INJ,$this,LA)
	set la [lindex $la 0]

	set rc [lindex $la 1]
	set de [lindex $la 2]

	if { $rc == "" || $de == "" || $rc == 0 } {
		alert "No repeat parameters for this layout"
		return
	}

	# sent count+ delay
	set WN(INJ,$this,RL) $rc
	set WN(INJ,$this,RK) 0
	set WN(INJ,$this,RD) $de

	set WN(INJ,$this,CB) [after 100 "inject_repeat_callback $this"]
	inject_button_status
}

proc inject_repeat_active { this } {
#
	global WN

	if { [info exists WN(INJ,$this,WN)] && $WN(INJ,$this,CB) != "" } {
		return 1
	}

	return 0
}

proc inject_repeat_callback { this } {
#
	global WN PM

	if ![inject_repeat_active $this] {
		# we are gone
		return
	}

	set pkt [inject_build_packet $this]

	if { $pkt == "" } {
		# problems
		set WN(INJ,$this,CB) ""
		inject_button_status
		alert "Cannot build packet from this layout"
		return
	}

	set rc [expr $WN(INJ,$this,RK) + 1]

	# this is best effort (should we change?)
	set seq [rqc]
	noss_send "[binary format ccc 0x08 $seq [string length $pkt]]$pkt"

	log "** injected packet \[[tstamp]\] ($rc of $WN(INJ,$this,RL),\
		length=[string length $pkt]):"
	log "=> [string trimleft [hencode $pkt]]"

	if { $rc >= $WN(INJ,$this,RL) } {
		# done
		set WN(INJ,$this,CB) ""
		inject_button_status
		return
	}

	set WN(INJ,$this,RK) $rc
	set WN(INJ,$this,CB) \
		[after $WN(INJ,$this,RD) "inject_repeat_callback $this"]
}

###############################################################################

proc set_status { stat } {

	global ST

	if { $ST(CST) != $stat } {
		wm title . "Olsonet Spectrum Analyzer V$ST(VER): $stat"
		set ST(CST) $stat
	}
}

###############################################################################
	
proc trc { msg } {

	global ST

	if !$ST(DBG) {
		return
	}

	puts $msg
}

proc dmp { hd buf } {

	global ST

	if !$ST(DBG) {
		return
	}

	puts "$hd ->[hencode $buf]"
}

proc initialize { } {

	global ST PM argv

	unames_init $ST(DEV) $ST(SYS)

	# explicit device list
	set edl ""

	set ar $argv
	while { $ar != "" } {
		set a [lindex $ar 0]
		if { [string index $a 0] != "-" } {
			lappend edl $a
		} elseif { $a == "-D" } {
			set ST(DBG) 1
		} elseif { $a == "-C" } {
			# explicit config file
			set ar [lrange $ar 1 end]
			set a [lindex $ar 0]
			if { $a != "" } {
				set PM(RCF) $a
			}
		}
		set ar [lrange $ar 1 end]
	}

	# trc "Starting"

	getsparams

	mk_rootwin .

	set_status "disconnected"

	update_widgets

	redraw

	return $edl
}

###############################################################################
###############################################################################

tip_init

# UART FD
set ST(SFD) 	""

# UART device
set ST(UDV)	""

# Handshake status
set ST(HSK)	0

# Debug flag
set ST(DBG)	0

# maximum message length
set PM(MPL)	132

# UART bit rate
set PM(DSP) 	115200

# tip text wrap length (pixels)
set PM(TWR)	320

# soft delay before closing the UART to give the node a chance to stop
set PM(SDL)	8000

# number of retries for commands
set PM(RET)	3

# Handshake timeout: short, long
set PM(HST)	{ 2000 8000 }

# scan in progress
set ST(SIP)	0

# signal scale mode 0-RSSI, 1-dB
set ST(SSM)	0

# request counter
set ST(RQC)	0

# expected command list, actual command code, or -1 for timeout, the message,
# timer
set ST(ECM)	""
set ST(ECM,C)	0
set ST(ECM,M)	""
set ST(ECM,T)	""

###############################################################################

# character zero (aka NULL)
set CH(ZER)	[format %c [expr 0x00]]

###############################################################################

# tag for unique identifiers
set WN(UNQ) 0

# canvas margins: left, right, bottom, up
set WN(LMA) 5
set WN(RMA) 5
set WN(BMA) 25
set WN(UMA) 5

# number of samples in a row, number of single-freq samples in a packet
set PM(NSA) 129
set PM(NSS) [expr $PM(NSA) - 1]

# maximum injected packet length
set PM(MPS) 63

# sample expansion factor: pixels per sample
set WN(SEF) 4

# drawable canvas width and height (total will include margins)
set WN(CAW) [expr $PM(NSA) * $WN(SEF)]
set WN(CAH) 512

# minimum separation between tick on horizontal axis
set WN(MHT) 70

# minimum separation between ticks on vertical axis
set WN(MVT) 50

# x-tick height
set WN(XTS) 4

# y-tick 1/2 width
set WN(YTS) 2

# axis color
set WN(AXC) "#FFFF66"

# sample color
set WN(SMC) "#00FF00"

# max color
set WN(MXC) "#FF0000"

# RO register label color
set WN(RRC) "#DDDDDD"

# RW register label color
set WN(RWC) "#A6D0FC"

# R-button color
set WN(RBC) "#F9D6A4"

# W-button color
set WN(WBC) "#FCADA6"

# x,y-offsets + color of peak text
set WN(PXF) 12
set WN(PYF) 16
set WN(PCO) "#00FF00"

# slider width
set WN(SLW) 400

# number of layout entries in the inject window
set WN(NLE) 12

# width of "content" label in a layout entry (in characters); it should be a
# multiple of 3 - 1; the second limit is the maximum number of bytes that can
# be displayed in the label
set WN(NCC) 23
set WN(NCM) [expr ($WN(NCC) + 1) / 3]

# log buffer size (number of lines); later we will turn this into a flexible
# parameter
set PM(MXL) 1024

# the log, and number of lines
set ST(LOG) ""
set ST(LOL) 0

# current log file + descriptor
set ST(LOF) ""
set ST(LFD) ""

# displayable connection status + received (raw) message counter
set ST(CST) ""
set ST(RRM) 0

# log text area
set WN(LOG) ""

# regs window
set WN(REG) ""

# cursors for regs pane and clickable digit
set WN(CRP) plus
set WN(CCD) hand2

# log autostart menu
set WN(ASM) { "off" "on" "on+file" }

# rc file
set PM(RCF) ".sapicrc"
if { [info exists env(HOME)] && $env(HOME) != "" } {
	set e $env(HOME)
} else {
	set e [pwd]
}

set PM(RCF) [file join $e $PM(RCF)]

# default autostart; values 0 1
set PM(AST_def) 0

# default autolog; values 0 1 2 (2 = also to the file)
set PM(LON_def) 0

# default log file
set PM(LOF_def) ""

# frequency prereqs: minimum, maximum (from), maximum (to) default, min step
# max step, crystal freq
set PM(FRE_min) 300.00
set PM(FRE_max) 930.00
set PM(FRE_def) 904.00
set PM(FRE_qua) 26.0

# Samples to average, inter-sample delay
set PM(STA_max) 255
set PM(ISD_max) 255
set PM(EMA_max) 64
set PM(MOA_max) $PM(NSS)

# List of discrete bands to choose from (MHz)
set PM(BAN) { 0.025 0.05 0.1 0.2 0.5 1.0 2.0 5.0 10.0 20.0 50.0 }

# List of discrete sizes for inject packet layout
set PM(LAS) { - }
for { set e 1 } { $e <= $PM(MPS) } { incr e } {
	lappend PM(LAS) $e
}

# list of value types
set PM(VTY) { hex int oct str }

proc setsmfont { t } {

	foreach e [font names] {
		if [regexp -nocase $t $e] {
			return $e
		}
	}

	return ""
}

# font for axis ticks #########################################################

set FO(SMA) [setsmfont icon]
if { $FO(SMA) == "" } {
	set FO(SMA) [setsmfont menu]
}
if { $FO(SMA) == "" } {
	set FO(SMA) [setsmfont small]
}

if { $FO(SMA) == "" } {
	set FO(SMA) {-family courier -size 9}
}

###############################################################################

# font for the log
set FO(LOG) "-family courier -size 9"

# for register labels
set FO(REG) "-family courier -size 10"

# for packet being injected
set FO(INJ) "-family courier -size 10"

unset e

set Params ""

proc pardef { name vcmd def } {

	global RC Params

	set RC(+,$name) $vcmd
	set RC($name) $def

	lappend Params $name
}

# list of RC parameters together with validation functions and default
# (fallback) values

pardef AST "valid_int_number %I 0 1 %O" $PM(AST_def)
pardef LON "valid_int_number %I 0 2 %O" $PM(LON_def)
pardef LOF "" $PM(LOF_def)
pardef FCE "valid_fp_number %I $PM(FRE_min) $PM(FRE_max) 2 %O" $PM(FRE_def)
pardef BAN "valid_ban %I %O" [lindex $PM(BAN) 7]
pardef STA "valid_int_number %I 1 $PM(STA_max) %O" 1
pardef ISD "valid_int_number %I 0 $PM(ISD_max) %O" 1
pardef GRS "valid_int_number 0 128 %I %O" 64
pardef MRS "valid_int_number 128 255 %I %O" 255
pardef EMA "valid_int_number 1 $PM(EMA_max) %I %O" 1
pardef MOA "valid_int_number 1 $PM(MOA_max) %I %O" 1
pardef TMX "valid_int_number 0 1 %I %O" 1

###############################################################################
###############################################################################

set WN(CRG) "Built In"

set t0 "Invert output, i.e. select active low"
set t1 "See table 41 in the manual (also see IOCFG2 CFG)."

regdef	IOCFG2	     0x00 0x2f
regtip	IOCFG2		"GDO2 output pin configuration."

regfld	IOCFG2 INV 6
regftp	IOCFG2 INV	$t0

regfld	IOCFG2 CFG 5 0
regftp	IOCFG2 CFG	"See table 41 in the manual. Some initial values\
			 are:\n0 - asserts when RX FIFO is filled at or above\
			 the RX FIFO threshold\n1 - asserts when RX FIFO is\
			 filled at or above the RX FIFO threshold or the end\
			 of packet is reached\n2 - TX FIFO filled at or above\
			 the TX FIFO threshold\n3 - TX FIFO full\n4 - RX FIFO\
			 overflow\n5 - TX FIFO underflow\n6 - sync word\
			 sent/received\n7 - packet has been received with CRC\
			 OK\n8 - preamble quality reached\n9 - clear channel\
			 assessment."

###############################################################################

regdef	IOCFG1	     0x01 0x2f
regtip	IOCFG1		"GDO1 output pin configuration."

regfld	IOCFG1 INV 6
regftp	IOCFG1 INV	$t0

regfld	IOCFG1 CFG 5 0
regftp	IOCFG1 CFG	$t1


###############################################################################

regdef	IOCFG0	     0x02 0x01
regtip	IOCFG0		"GDO0 output pin configuration."

regfld	IOCFG0 INV 6
regftp	IOCFG0 INV	$t0

regfld	IOCFG0 CFG 5 0
regftp	IOCFG0 CFG	$t1

unset t0 t1

###############################################################################

regdef	FIFOTHR	     0x03 0x0f
regtip	FIFOTHR		"FIFO thresholds."

regfld	FIFOTHR RET 6
regftp	FIFOTHR RET	"0: TEST1=0x31 and TEST2=0x88 when waking up from\
			 SLEEP. 1: TEST1=0x35 and TEST2=0x81 when waking up\
			 from SLEEP. Note that the changes in the TEST\
			 registers due to the RET setting are only seen\
			 INTERNALLY in the analog part. The values read from\
			 the TEST registers when waking up from SLEEP mode\
			 will always be the reset values. The RET bit should be\
			 set to 1 before going into SLEEP mode if settings\
			 with an RX filter bandwidth below 325 kHz are required\
			 at wake-up."

regfld	FIFOTHR ATT 5 4
regftp	FIFOTHR ATT	"RX attenuation (close-range reception):\n00 - 0dB\n01\
			 - 6dB\n10 - 12dB\n11 - 18dB."

regfld	FIFOTHR THR 3 0
regftp	FIFOTHR THR	"Threshold for the TX FIFO and RX FIFO.\
			 The threshold is exceeded when the number of bytes in\
			 the FIFO is equal to or higher than the threshold\
			 value:\n0 - TX:61, RX:4\n1 - 57, 8\n2 - 53, 12\n3 -\
			 49, 16\n4 - 45, 20\n5 - 41, 24\n6 - 37, 28\n7 - 33,\
			 32\n8 - 29, 36\n9 - 25, 40\n10 - 21, 44\n11 - 17,\
			 48\n12 - 13, 52\n13 - 9, 56\n14 - 5, 60\n15 - 1, 64."

###############################################################################

regdef	SYNC1        0x04 0xAB
regtip	SYNC1		"Sync word upper byte."

###############################################################################

regdef	SYNC0        0x05 0x35
regtip	SYNC0		"Sync word lower byte."

###############################################################################

regdef	PKTLEN       0x06 63
regtip	PKTLEN		"Indicates the packet length when fixed packet length\
			 mode is enabled.\nIf variable packet length mode is\
			 used, this value indicates the maximum packet length\
			 allowed.\nMust be different from 0.\nThe default\
			 setting for PKTLEN is max FIFO-1; this is because the\
			 default packet params are for \"formatted\" packets\
			 with the length sent as the first byte."

###############################################################################

regdef	PKTCTRL1     0x07 0x04
regtip	PKTCTRL1	"Packet automation control."

regfld	PKTCTRL1 PQT 7 5
regftp	PKTCTRL1 PQT	"Preamble quality estimator threshold\
			 (4*PQT). Higher values increase preamble quality\
			 requirements before\
			 accepting sync word.\nIf PQT is zero, then sync is\
			 always accepted.\nThe preamble quality estimator\
			 increases an internal counter by one each time a bit\
			 is received that is different from the previous bit,\
			 and decreases the counter by 8 each time a bit is\
			 received that is the same as the last bit."

regfld	PKTCTRL1 CRCAF 3
regftp	PKTCTRL1 CRCAF	"CRC autoflush. Theoretically enables automatic\
			 cleanup of FIFO when hardware CRC is bad. Doesn't\
			 seem to work, so don't use."

regfld	PKTCTRL1 AS 2
regftp	PKTCTRL1 AS	"Append status: when enabled, two status bytes will be\
			 appended to the payload of the packet. The status\
			 bytes contain RSSI and LQI values, as well as the\
			 CRC OK flag."

regfld	PKTCTRL1 ACHK 1 0
regftp	PKTCTRL1 ACHK	"Controls address check configuration of received\
			 packets:\n00 - no check\n01 - check,\
			 no broadcast\n10 -\
			 check, 0x00 = bdcst\n11 - check, 0x00+0xff are\
			 bdcst\nWe don't use this feature."

###############################################################################

regdef	PKTCTRL0     0x08 0x45
regtip	PKTCTRL0	"Packet automation control."

regfld	PKTCTRL0 WHITE 6
regftp	PKTCTRL0 WHITE	"Turns data whitening on."

regfld	PKTCTRL0 FMT 5 4
regftp	PKTCTRL0 FMT	"Packet format, 00 = normal mode using FIFOs for both\
			 RX and TX. Don't use any other values."

regfld	PKTCTRL0 CRC 2
regftp	PKTCTRL0 CRC	"Enable hardware CRC (calculation and check)."

regfld	PKTCTRL0 LCF 1 0
regftp	PKTCTRL0 LCF	"Packet length configuration: 00 = fixed length (as\
			 set in PKTLEN), 01 = variable (length in first byte\
			 after sync), 10 = infinite, 11 = unused.\nWe only use\
			 01."

###############################################################################

regdef	ADDR         0x09 0x00
regtip	ADDR		"Device address used for packet filtration.\
			 Optional broadcast addresses are 0x00 and 0xFF."

###############################################################################

regdef	CHANNR       0x0A 0x00
regtip	CHANNR		"Channel number (multiplied by channel spacing and\
			 added to the base frequency). See MDMCFG0."

###############################################################################

regdef	FSCTRL1      0x0B 0x0C
regtip	FSCTRL1		"Frequency synthesizer control."

regfld	FSCTRL1 IF 4 0
regftp	FSCTRL1 IF	"The IF (intermediate frequency) to be used by the\
			 receiver. Subtracted from the FS base frequency.\
			 Controls the digital complex mixer in the\
			 demodulator.\nFrequency = (Fxtal * IF) / 1024."

###############################################################################

regdef	FSCTRL0      0x0C 0x00
regtip	FSCTRL0		"Frequency synthesizer control.\nFrequency offset\
			 added to the base frequency before being used by the\
			 synthesizer (2's-complement).\
			 Resolution is Fxtal/2^14 (1.59kHz-1.65kHz);\
			 range is +-202 kHz to +-210 kHz, dependent of XTAL\
			 frequency."

###############################################################################

set tip			"The frequency is\
			 determined as:\n\Fc = (Fxtal * FREQ)/65536\nwhere\
			 FREQ is FREQ2.FREQ1.FREQ0 (a 24 byte int)."

regdef	FREQ2        0x0D 0x22
regtip	FREQ2		"Frequency control word (high byte). $tip"
			
regdef	FREQ1        0x0E 0xC4
regtip	FREQ1		"Frequency control word (middle byte). $tip"

regdef	FREQ0        0x0F 0xEC
regtip	FREQ0		"Frequency control word (low byte). $tip"

unset tip

###############################################################################

regdef	MDMCFG4      0x10 0x68
regtip	MDMCFG4		"Modem configuration."

regfld	MDMCFG4 CHBW 7 4
regftp	MDMCFG4 CHBW	"These are in fact two fields (E7:6 and M5:4) which\
			 together determine the channel filter bandwidth, i.e.,\
			 the selectivity of the channel. You can reduce this\
			 (to improve sensitivity), e.g., if you know that the\
			 match is good, or you have compensated by an offset\
			 (see FSCTRL0). As a single value, they yield (kHz):\
			 0-812, 1-650, 2-541, 3-464, 4-406, 5-325, 6-270,\
			 7-232, 8-203, 9-162, 10-135, 11-116, 12-102, 13-81,\
			 14-68, 15-58."

regfld	MDMCFG4 DRE 3 0
regftp	MDMCFG4 DRE	"The exponent of data rate, see MDMCFG3."

###############################################################################

regdef	MDMCFG3      0x11 0x93
regtip	MDMCFG3		"Modem configuration: the mantissa of the data rate\
			 (the exponent is given\
			 by DRE (see MDMCFG4). The rate is determined as:\nR\
			 = (256 + M) * 2^E * Fxtal / 2^28."

###############################################################################

regdef	MDMCFG2      0x12 0x03
regtip	MDMCFG2		"Modem configuration."

regfld	MDMCFG2 DCOFF 7
regftp	MDMCFG2 DCOFF	"Disable DC blocking filter (for some current savings).\
			 We don't disable the filter. Disabling the filter\
			 affects the recommended IF setting."

regfld	MDMCFG2 MODF 6 4
regftp	MDMCFG2 MODF	"Modulation format:\n000 - 2-FSK\n001 - GFSK\n011\
			 - ASK/OOK\n100 - 4-FSK\n111 - MSK.\nMSK is only\
			 supported for data rates above 26k."

regfld	MDMCFG2 MANC 3
regftp	MDMCFG2 MANC	"Enables Manchester encoding."

regfld	MDMCFG2 SYNC 2 0
regftp	MDMCFG2 SYNC	"Sync word/qualifier mode:\n000 - no\
			 preamble/sync\n001 - 15/16 sync bits detected\n010 -\
			 16/16 sync bits detected\n011 - 30/32 sync bits\
			 detected\n100 - no pre/sync, CS above\
			 threshold\n101 - 15/16 + CS above threshold\n110 -\
			 16/16 + CS above threshold\n111 - 30/32 + CS above\
			 threshold.\nThe values 0 and 4 disable preamble and\
			 sync word transmission in TX and preamble and sync\
			 word detection in RX. The values 1, 2, 5, and 6\
		 	 enable 16-bit sync word transmission in TX and\
			 16 bits detection in RX. Only 15 of 16 bits need to\
			 match in RX when using settings 1 or 5. The values 3\
			 and 7 enable repeated sync word transmission in TX\
			 and 32-bit sync word detection in RX (only 30 of 32\
			 bits need to match)."

###############################################################################

regdef	MDMCFG1      0x13 0x42
regtip	MDMCFG1		"Modem configuration."

regfld	MDMCFG1 FEC 7
regftp 	MDMCFG1 FEC	"Enable forward error correction. Only supported in\
			 fixed packet length mode (see PKTCTRL0 LCF)."

regfld	MDMCFG1 PRE 6 4
regftp	MDMCFG1 PRE	"Sets the minimum number of preamble bytes to be\
			 transmitted: 0-2, 1-3, 2-4, 3-6, 4-8, 5-12, 6-16,\
			 7-24."

regfld	MDMCFG1 CSE 1 0
regftp	MDMCFG1 CSE	"Channel spacing exponent, see MDMCFG0."

###############################################################################

regdef	MDMCFG0      0x14 0xF8
regtip	MDMCFG0		"Modem configuration: channel spacing mantissa.\
			 The formula is:\nSP = (256 + M) * 2^E * Fxtal / 2^18."

###############################################################################

regdef	DEVIATN      0x15 0x34
regtip	DEVIATN		"Modem deviation setting."

regfld	DEVIATN E 6 4
regftp	DEVIATN E	"Deviation exponent."

regfld	DEVIATN M 2 0
regftp	DEVIATN M	"Deviation mantissa. For TX with\
			 2-FSK, GFSK, 4-FSK, M and E\
			 specifiy the nominal frequency deviation from the\
			 carrier for a 0 (-DEVIATN) and 1 (+DEVIATN) as\nD =\
			 (8 + M) * 2^E * Fxtal / 2^17.\nFor MSK specifies the\
			 fraction of symbol period (1/8-8/8) during which a\
			 phase change occurs (0: +90deg, 1:-90deg).\nFor RX\
			 with 2-FSK, GFSK, 4-FSK, specifies the expected\
			 deviation of incoming signal.\nFor ASK/OOK this\
			 setting has no effect."

###############################################################################

set t0 "Main radio control state machine configuration."

regdef	MCSM2        0x16 0x07
regtip	MCSM2		$t0

regfld	MCSM2 RSSI 4
regftp 	MCSM2 RSSI	"Direct RX termination based on RSSI measurement (CS).\
			 For ASK/OOK modulation, RX times out if there is no\
			 CS in the first 8 symbol periods. See AGCCTRL1,2."

regfld	MCSM2 QUAL 3
regftp	MCSM2 QUAL	"When TIME expires, the chip checks if sync word is\
			 found (when QUAL=0), or if either sync word is found\
			 or PQI (preamble qualifier, see MDMCFG2 SYNC) is set\
			 (when QUAL=1)."

regfld	MCSM2 TIME 2 0
regftp	MCSM2 TIME	"Timeout for sync word search in RX for both WOR mode\
			 and normal RX operation. The timeout is relative to\
			 the programmed EVENT0 timeout. Note: we don't use WOR."

###############################################################################

regdef	MCSM1        0x17 0x03
regtip	MCSM1		$t0

regfld	MCSM1 CCA 5 4
regftp	MCSM1 CCA	"Selects CCA (clear channel assessment) mode for\
			 RX:\n00 - always\n01 - RSSI below threshold\n10 -\
			 not receiving\n11 - both."

regfld	MCSM1 RXOFF 3 2
regftp	MCSM1 RXOFF	"What happens after packet reception (the module's\
			 state):\n00 - IDLE\n01 - FSTXON\n10 - TX\n11 -\
			 RX\nIt is not possible to set RXOFF to TX or FSTXON\
			 and at the same time use CCA."

regfld	MCSM1 TXOFF 1 0
regftp	MCSM1 TXOFF	"What happens after packet transmission (the module's\
			 state):\n00 - IDLE\n01 - FSTXON\n10 - TX\n11 - RX."
			
###############################################################################

regdef	MCSM0        0x18 0x18
regtip	MCSM0		$t0

regfld	MCSM0 CAL 5 4
regftp	MCSM0 CAL	"Automatically calibrate when going to RX or TX, or\
			 back to IDLE:\n00 - never (only manually)\n01 -\
			 when going from IDLE to RX or TX\n10 -\
			 when going from RX or TX to IDLE\n11 - every 4th time\
			 when going from RX/TX to IDLE."

regfld	MCSM0 TIM 3 2
regftp	MCSM0 TIM	"Timeout to stabilize oscillator on power up:\n00 -\
			 2.3-3.4us\n01 - 37-39us\n10 - 149-155us\n11 -\
			 597-620us."

regfld	MCSM0 PEN 1
regftp	MCSM0 PEN	"Enables the pin radio control option."

regfld	MCSM0 XON 0
regftp	MCSM0 XON	"Forces the crystal oscillator to stay on in the\
			 SLEEP state."

unset t0

###############################################################################

regdef	FOCCFG       0x19 0x15
regtip	FOCCFG		"Frequency offset compensation configuration."

regfld	FOCCFG FRZ 5
regftp	FOCCFG FRZ	"Freeze offset compensation until CS goes high\
			 (according to the CS thresholding described in\
			 AGCCTRL1,2). Useful when there are long gaps\
			 between reception with RF switched on (otherwise\
			 offset may go to boundaries while tracking noise)."

regfld	FOCCFG SYN 4 3
regftp	FOCCFG SYN	"Frequency compensation loop gain to be used\
			 before a sync word is detected:\n00 - K\n01 -\
			 2K\n10 - 3K\n11 - 4K."

regfld	FOCCFG POST 2
regftp	FOCCFG POST	"Frequency compensation loop gain to be used after\
			 a sync word is detected:\n0 - same as SYN\n1 - K/2."

regfld	FOCCFG LIMIT 1 0
regftp	FOCCFG LIMIT	"Saturation point for the compensation algorithm\
			 (when to stop):\n00 - switched off\n01 -\
			 +-BW/8\n10 - +-BW/4\n11 - +-BW/2\nwhere\
			 BW is the channel filter bandwidth (MDMCFG4)."

###############################################################################

regdef	BSCFG        0x1A 0x6C
regtip	BSCFG		"Bit synchronization configuration."

regfld	BSCFG BPI 7 6
regftp	BSCFG BPI	"Clock recovery feedback loop integral gain to be used\
			 before a sync word is detected (to correct offsets in\
			 data rate):\n00 - Kl\n01 - 2Kl\n10 - 3Kl\n11 - 4Kl."

regfld	BSCFG BPP 5 4
regftp	BSCFG BPP 	"Clock recovery feedback loop proportional gain to be\
			 used before a sync word is detected:\n00 - Kp\n01 -\
			 2Kp\n10 - 3Kp\n11 - 4Kp."

regfld	BSCFG BAI 3
regftp	BSCFG BAI	"Clock recovery feedback loop integral gain to be used\
			 after a sync word is detected:\n0 - same as BPI\n1 -\
			 Kl/2."

regfld	BSCFG BAP 2
regftp	BSCFG BAP	"Clock recovery feedback loop proportional gain to be\
			 used after a sync word is detected:\n0 - same as\
			 BPP\n1 - Kp."

regfld	BSCFG LIMIT 1 0
regftp	BSCFG LIMIT	"Saturation point (max data rate difference) for the\
			 data rate offset compensation algorithm:\n00 -\
			 +- 0 (no compensation)\n01 - +-3.125%%\n10 -\
			 +-6.25%%\n11 - +-12.5%%."

###############################################################################

regdef	AGCCTRL2     0x1B 0x03
regtip	AGCCTRL2	"AGC control: primarily determining CS for clear\
			 channel assessment, but also for triggering CS-based\
			 reception events, if used."

regfld	AGCCTRL2 DVGA 7 6
regftp	AGCCTRL2 DVGA	"Reduces the maximum allowable DVGA gain:\n00 - all\
			 settings used\n01 - except for highest\n10 - except\
			 for 2 highest\n11 - except for 3 highest."

regfld	AGCCTRL2 LNA 5 3
regftp	AGCCTRL2 LNA	"Sets the maximum allowable LNA + LNA 2 gain relative\
			 to the maximum possible gain:\n0 - max possible\n1 -\
			 2.6dB less\n2 - 6.1dB less\n3 - 7.4dB less\n4 -\
			 9.2dB less\n5 - 11.5dB less\n6 - 14.6dB less\n7 -\
			 17.1dB less."

regfld	AGCCTRL2 TGT 2 0
regftp	AGCCTRL2 TGT	"Target for the averaged amplitude from the digital\
			 channel filter (1 LSB = 0 dB):\n0 - 24dB\n1 -\
			 27dB\n2 - 30dB\n3 - 33dB\n4 - 36dB\n5 - 38dB\n6 -\
			 40dB\n7 - 42dB."

###############################################################################

set t0 "AGC control."

regdef	AGCCTRL1     0x1C 0x40
regtip	AGCCTRL1	$t0

regfld	AGCCTRL1 LNA 6	
regftp	AGCCTRL1 LNA	"Selects between two different strategies for LNA and\
			 LNA 2 gain adjustment. When 1, the LNA gain is\
			 decreased first. When 0, the LNA 2 gain is decreased\
			 to minimum before decreasing LNA gain."

regfld	AGCCTRL1 CSR 5 4
regftp	AGCCTRL1 CSR	"Relative change threshold for asserting carrier\
			 sense:\n00 - disabled\n01 - 6dB increase\n10 -\
			 10dB increase\n11 - 14dB increase."

regfld	AGCCTRL1 CSA 3 0
regftp	AGCCTRL1 CSA	"Absolute RSSI threshold for asserting carrier sense\
			 (2-complement, signed, in steps of 1 dB, relative to\
			 AGGCTRL2 TGT):\n1000 (-8) - disabled\n1001 (-7) -\
			 7dB below TGT\n...\n1111 (-1) - 1dB below TGT\n0000 -\
			 at TGT\n0001 - 1dB above TGT\n...\n0111 - 7dB above\
			 TGT."

###############################################################################

regdef	AGCCTRL0     0x1D 0x91
regtip	AGCCTRL0	$t0

regfld	AGCCTRL0 HYST 7 6
regftp	AGCCTRL0 HYST	"Level of hysteresis on the magnitude deviation\
			 (internal AGC signal that determines gain\
			 changes):\n0 - no hysteresis, small symmetric dead\
			 zone, high gain\n1 - low hysteresis, small\
			 asymmetric dead zone, medium gain\n2 - medium\
			 hysteresis, medium asymmetric dead zone, medium\
			 gain\n3 - large hysteresis, large asymmetric dead\
			 zone, low gain."

regfld	AGCCTRL0 WAIT 5 4
regftp	AGCCTRL0 WAIT	"The number of channel filter samples from a gain\
			 adjustment to be made until the AGC algorithm starts\
			 accumulating new samples:\n00 - 8\n01 - 16\n10 -\
			 24\n11 - 32."

regfld	AGCCTRL0 FRE 3 2
regftp	AGCCTRL0 FRE	"Freeze AGC:\n00 - never\n01 - when\
			 a sync word detected\n10 - manually freeze analogue\
			 gain, continue to adjust digital gain\n11 - freeze\
			 both gains."

regfld	AGCCTRL0 FLE 1 0
regftp	AGCCTRL0 FLE	"For 2-FSK, 4-FSK, MSK: the averaging length (number of\
			 samples) for the amplitude from the channel\
			 filter: 0 - 8, 1 - 16, 2 - 32, 3 - 64.\nFor ASK, OOK:\
			 the decision boundary for reception: 0 - 4dB, 1 - \
			 8dB, 2 - 12dB, 3 - 16dB."

unset t0

###############################################################################

regdef	WOREVT1	     0x1E 0x87
regtip	WOREVT1		"High byte event0 timeout.\nTe0 =\
			 750 * T * 2^RES / Fxtal\nSee WORCTRL for RES."

###############################################################################

regdef	WOREVT0	     0x1F 0x6B
regtip	WOREVT0		"Low byte event0 timeout. See WOREVT1."

###############################################################################

regdef	WORCTRL	     0x20 0x01
regtip	WORCTRL		"Wake on radio control."

regfld	WORCTRL PD 7
regftp	WORCTRL PD	"Power down signal to RC oscillator. When set to 0,\
			 automatic initial calibration will be performed."

regfld	WORCTRL EV 6 4
regftp	WORCTRL EV	"Timeout setting from register block (decoded to Event1\
			 timeout. RC oscillator clock frequency = Fxtal/750 =\
			 34.7kHz. The number of clock periods after Event0\
			 before Event 1 times out is:\n0 - 4 (0.111ms)\n1 -\
			 6 (0.167ms)\n2 - 8 (0.222ms)\n3 - 12 (0.333ms)\n4 -\
			 16 (0.444ms)\n5 - 24 (0.667ms)\n6 - 32 (0.889ms)\n7 -\
			 48 (1.333ms)."

regfld	WORCTRL CAL 3
regftp 	WORCTRL CAL	"Enables the RC oscillator calibration."

regfld	WORCTRL RES 1 0
regftp	WORCTRL RES	"Controls Event0 resolution + the maximum timeout of\
			 the WOR module + the maximum timeout under normal RX\
			 operation:\n0 - resolution 1 period (28us), max\
			 timeout 1.8s\n1 - 32 periods (0.89us)/58s\n2 -\
			 1024 periods (28ms)/31m\n3 - 32K periods\
			 (0.91s)/16.5h."

###############################################################################

regdef	FREND1       0x21 0x56
regtip	FREND1		"Front end RX configuration."

regfld	FREND1 LNA 7 6
regftp	FREND1 LNA	"Adjusts front-end LNA PTAT current output."

regfld	FREND1 PTAT 5 4
regftp	FREND1 PTAT	"Adjusts front-end PTAT outputs."

regfld	FREND1 LODIV 3 2
regftp	FREND1 LODIV	"Adjusts current in RX LO buffer (LO input to mixer)."

regfld	FREND1 MIX 1 0
regftp	FREND1 MIX	"Adjusts current in mixer."

###############################################################################

regdef	FREND0       0x22 0x17
regtip	FREND0		"Front end TX configuration."

regfld	FREND0 LODIV 5 4
regftp	FREND0 LODIV	"Adjusts current TX LO buffer (input to PA)."

regfld	FREND0 PAP 2 0
regftp	FREND0 PAP	"Selects PA power setting. This is an index to the\
			 PATABLE, which can be programmed with up to 8\
			 different PA settings. In OOK/ASK mode, this selects\
			 the PATABLE index to use when transmitting a 1.\
			 PATABLE index zero is used in OOK/ASK when\
			 transmitting a 0. The PATABLE settings from index 0\
			 to PAP are used for ASK TX shaping, and for power\
			 ramp-up/ramp-down at the start/end of transmission\
			 in all TX modulation formats."

###############################################################################

regdef	FSCAL3       0x23 0xA9
regtip	FSCAL3		"Frequency synthesizer calibration."

regfld	FSCAL3 CONF 7 6
regftp	FSCAL3 CONF	"Some configuration attribute. This is basically\
			 magic. The manual says nothing"

regfld	FSCAL3 EN 5 4
regftp	FSCAL3 EN	"Set to zero to disable charge pump calibration stage."

regfld	FSCAL3 RES 3 0
regftp	FSCAL3 RES	"Frequency synthesizer calibration result register.\
			 Digital bit vector defining the charge pump output\
			 current, on an exponential scale:\nIout =\
			 I0 * 2^(RES/4)\nFast frequency hopping without\
			 calibration for each hop can be done by calibrating\
			 upfront for each frequency and saving the resulting\
			 FSCAL3, FSCAL2 and FSCAL1 register values.\nBetween\
			 each frequency hop, calibration can be replaced by\
			 writing the FSCAL3, FSCAL2 and FSCAL1 register values\
			 corresponding to the next RF frequency."

###############################################################################

set t0 "Frequency synthesizer calibration."

regdef	FSCAL2       0x24 0x2A
regtip	FSCAL2		$t0

regfld	FSCAL2 VCO 5
regftp	FSCAL2 VCO	"Chooses high (1) or low (0) VCO (voltage\
			 controlled oscillator)."

regfld	FSCAL2 RES 4 0
regftp	FSCAL2 RES	"Result register (see FSCAL3 RES)."

###############################################################################

regdef	FSCAL1       0x25 0x00
regtip	FSCAL1		$t0

regfld	FSCAL1 RES 5 0
regftp	FSCAL1 RES	"Result register (see FSCAL3 RES). Capacitor array\
			 setting for VCO coarse tuning."

###############################################################################

regdef	FSCAL0       0x26 0x0D
regtip	FSCAL0		"$t0 This is some magic\
			 of which the manual says nothing."

unset t0

###############################################################################

regdef	RCCTRL1      0x27 0x00
regtip	RCCTRL1		"RC oscillator configuration.\nIn applications where\
			 the radio wakes up very often, it is possible to do\
			 the RC oscillator calibration once and then turn off\
			 calibration to reduce the current consumption. This\
			 is done by setting WORCTRL CAL to 0 and requires that\
			 RC oscillator calibration values are read from\
			 registers RCCTRL0S (status) and RCCTRL1S and\
			 written back to RCCTRL0 and RCCTRL1 respectively."

###############################################################################

regdef	RCCTRL0      0x28 0x00
regtip	RCCTRL0		"RC oscillator configuration. See RCCTRL1."

###############################################################################

regdef	FSTEST       0x29 0x59
regtip	FSTEST		"For tests only, do not write!"

###############################################################################

regdef	PTEST        0x2A 0x7F
regtip	PTEST		"Production test. Writing 0xBF to this register makes\
			 the on-chip temperature sensor available in the IDLE\
			 state. The default 0x7F value should then be written\
			 back before leaving the IDLE state. Other use of this\
			 register is for test only."

###############################################################################

regdef	AGCTEST       0x2B 0x3F
regtip	AGCTEST		"For tests only, do not write!"

###############################################################################

regdef	TEST2         0x2C 0x88
regtip	TEST2		"For tests only, better do not touch!"

###############################################################################

regdef	TEST1         0x2D 0x31
regtip	TEST1		"For tests only, better do not touch!"

###############################################################################

regdef	TEST0         0x2E 0x02
regtip	TEST0		"For tests only (also some magic),\
			 better do not touch!"

###############################################################################

regdef	PATABLE       0x3E { 0x00 0x30 0x04 0x09 0x19 0x33 0x86 0xE0 } 8
regtip	PATABLE		"TX power table to convert the power index\
			 (see FREND0 PAP) to the actual setting."
###############################################################################

# reading this causes problems
#regdef	PARTNUM       0x30 0 0 1
#regtip	PARTNUM		"Chip part number."

###############################################################################

regdef	VERSION       0x31 0 0 1
regtip	VERSION		"Chip version number."

###############################################################################

regdef	FREQEST       0x32 0 0 1
regtip	FREQEST		"The estimated frequency offset (2s complement) of the\
			 carrier. Resolution is: Fxtal/2^14 (1.59kHz),\
			 range is +-202kHz.\nFrequency offset compensation is\
			 only supported for 2-FSK, GFSK, 4-FSK, and MSK."

###############################################################################

regdef	LQI           0x33 0 0 1
regtip	LQI		"Demodulator estimate of link quality."

regfld	LQI CRC 7
regftp	LQI CRC		"CRC OK."

regfld	LQI EST 6 0
regftp	LQI EST		"Estimates how easily a received signal can be\
			 demodulated. Calculated over the 64 symbols following\
			 the sync word."

###############################################################################

regdef	RSSI          0x34 0 0 1
regtip	RSSI		"Received signal strength indication."

###############################################################################

regdef	STATE         0x35 0 0 1
regtip	STATE		"Machine state:\n0x00-SLEEP\n0x01-IDLE\n0x02-XOFF\
			 \n0x03-VCOON_MC\n0x04-REGON_MC\n0x05-MANCAL\
			 \n0x06-VCOON\n0x07-REGON\n0x08-STARTCAL\n0x09-BWBOOST\
			 \n0x0A-FS_LOCK\n0x0B-IFADCON\n0x0C-ENDCAL\n0x0D-RX\
			 \n0x0E-RX_END\n0x0F-RX_RST\n0x10-TXRX_SWITCH\
			 \n0x11-RXFIFO_OVERFLOW\n0x12-FSTXON\n0x13-TX\
			 \n0x14-TX_END\n0x15-RXTX_SWITCH\
			 \n0x16-TXFIFO_UNDERFLOW."

###############################################################################

regdef	WORTIME1      0x36 0 0 1
regtip	WORTIME1	"High byte of WOR time."

###############################################################################

regdef	WORTIME0      0x37 0 0 1
regtip	WORTIME0	"Low byte of WOR time."

###############################################################################

regdef	PKTSTATUS     0x38 0 0 1
regtip	PKTSTATUS	"GDOx and packet status."

regfld	PKTSTATUS CRC 7
regftp	PKTSTATUS CRC	"CRC OK (cleared when extering RX)."

regfld	PKTSTATUS CS 6
regftp 	PKTSTATUS CS	"Carrier sense (cleared when entering IDLE)."

regfld	PKTSTATUS PQT 5
regftp 	PKTSTATUS PQT	"Preamble quality reached. If leaving RX state when\
			 this bit is set it will remain asserted until the chip\
			 re-enters RX state. The bit will also be cleared if\
			 PQI goes below the programmed PQT value (see\
			 PKTCTRL1 PQT)."

regfld	PKTSTATUS CCA 4
regftp 	PKTSTATUS CCA	"Channel is clear."

regfld	PKTSTATUS SFD 3
regftp 	PKTSTATUS SFD	"Start of frame delimiter. In RX, asserted\
			 when sync word has been received and de-asserted at\
			 the end of the packet. It will also deassert when\
			 a packet is discarded due to address or maximum length\
			 filtering or the radio enters RXFIFO overflow. In TX\
			 always reads as 0."

regfld	PKTSTATUS GDO2 2
regftp 	PKTSTATUS GDO2	"Current GDO2 value."

regfld	PKTSTATUS GDO0 0
regftp 	PKTSTATUS GDO0	"Current GDO0 value."

###############################################################################

#regdef	VCOVCDAC      0x39 0 0 1
#regtip	VCOVCDAC	"For tests only."

###############################################################################

regdef	TXBYTES       0x3A 0 0 1
regtip	TXBYTES		"Underflow and number of bytes in TX FIFO."

regfld	TXBYTES U 7
regftp	TXBYTES U	"TX FIFO undeflow."

regfld	TXBYTES N 6 0
regftp	TXBYTES N	"Number of bytes in TX FIFO."


###############################################################################

regdef	RXBYTES       0x3B 0 0 1
regtip	RXBYTES		"Overflow and number of bytes in RX FIFO."

regfld	RXBYTES O 7
regftp	RXBYTES O	"RX FIFO overflow."

regfld	RXBYTES N 6 0
regftp	RXBYTES N	"Number of bytes in RX FIFO."

###############################################################################

regdef	RCCTRL1S      0x3C 0 0 1
regtip	RCCTRL1S	"Contains the RCCTRL1 value from the last run of the\
			 RC oscillator calibration routine (see RCCTRL1)."

###############################################################################

regdef	RCCTRL0S      0x3D 0 0 1
regtip	RCCTRL0S	"Contains the RCCTRL0 value from the last run of the\
			 RC oscillator calibration routine (see RCCTRL0)."

###############################################################################
###############################################################################
###############################################################################

if 0 {

# Alpha 1.0 sniffer on Warsaw

set PREINIT { PARAMS {AST 1 LON 1 LOF {} FCE 868.03 BAN Single/rcv STA 4 ISD 1 GRS 64 MRS 255 EMA 1 MOA 1 TMX 1} REGISTERS {Default {{IOCFG2 47} {IOCFG1 47} {IOCFG0 1} {FIFOTHR 15} {SYNC1 171} {SYNC0 53} {PKTLEN 63} {PKTCTRL1 4} {PKTCTRL0 69} {ADDR 0} {CHANNR 0} {FSCTRL1 12} {FSCTRL0 0} {FREQ2 34} {FREQ1 196} {FREQ0 236} {MDMCFG4 202} {MDMCFG3 131} {MDMCFG2 3} {MDMCFG1 66} {MDMCFG0 248} {DEVIATN 52} {MCSM2 7} {MCSM1 3} {MCSM0 24} {FOCCFG 21} {BSCFG 108} {AGCCTRL2 3} {AGCCTRL1 64} {AGCCTRL0 145} {WOREVT1 135} {WOREVT0 107} {WORCTRL 1} {FREND1 86} {FREND0 23} {FSCAL3 169} {FSCAL2 42} {FSCAL1 0} {FSCAL0 13} {RCCTRL1 0} {RCCTRL0 0} {FSTEST 89} {PTEST 127} {AGCTEST 63} {TEST2 136} {TEST1 49} {TEST0 2} {PATABLE {3 28 87 142 133 204 198 195}}}} }

}

if 0 {

# Alpha 1.0 for CC430 USB (38400)

set PREINIT { PARAMS {AST 0 LON 1 LOF {} FCE 868.00 BAN Single/rcv STA 4 ISD 1 GRS 33 MRS 255 EMA 1 MOA 1 TMX 1} REGISTERS {CC430_868_38400 {{IOCFG2 47} {IOCFG1 41} {IOCFG0 1} {FIFOTHR 15} {SYNC1 171} {SYNC0 53} {PKTLEN 63} {PKTCTRL1 4} {PKTCTRL0 69} {ADDR 0} {CHANNR 0} {FSCTRL1 12} {FSCTRL0 0} {FREQ2 33} {FREQ1 98} {FREQ0 118} {MDMCFG4 202} {MDMCFG3 131} {MDMCFG2 3} {MDMCFG1 66} {MDMCFG0 248} {DEVIATN 52} {MCSM2 7} {MCSM1 3} {MCSM0 16} {FOCCFG 21} {BSCFG 108} {AGCCTRL2 3} {AGCCTRL1 64} {AGCCTRL0 145} {WOREVT1 135} {WOREVT0 107} {WORCTRL 1} {FREND1 86} {FREND0 23} {FSCAL3 169} {FSCAL2 42} {FSCAL1 0} {FSCAL0 13} {RCCTRL1 0} {RCCTRL0 0} {FSTEST 89} {PTEST 127} {AGCTEST 63} {TEST2 136} {TEST1 49} {TEST0 2} {PATABLE {3 28 87 142 133 204 198 195}}} Default {{IOCFG2 47} {IOCFG1 47} {IOCFG0 1} {FIFOTHR 15} {SYNC1 171} {SYNC0 53} {PKTLEN 63} {PKTCTRL1 4} {PKTCTRL0 69} {ADDR 0} {CHANNR 0} {FSCTRL1 12} {FSCTRL0 0} {FREQ2 34} {FREQ1 196} {FREQ0 236} {MDMCFG4 202} {MDMCFG3 131} {MDMCFG2 3} {MDMCFG1 66} {MDMCFG0 248} {DEVIATN 52} {MCSM2 7} {MCSM1 3} {MCSM0 24} {FOCCFG 21} {BSCFG 108} {AGCCTRL2 3} {AGCCTRL1 64} {AGCCTRL0 145} {WOREVT1 135} {WOREVT0 107} {WORCTRL 1} {FREND1 86} {FREND0 16} {FSCAL3 169} {FSCAL2 42} {FSCAL1 0} {FSCAL0 13} {RCCTRL1 0} {RCCTRL0 0} {FSTEST 89} {PTEST 127} {AGCTEST 63} {TEST2 136} {TEST1 49} {TEST0 2} {PATABLE {3 28 87 142 133 204 198 195}}}} }

}



###############################################################################
###############################################################################
###############################################################################

trigger

autocn_start uart_open noss_close send_handshake handshake_ok uart_connected \
	send_poll [initialize]


while 1 { event_loop }
