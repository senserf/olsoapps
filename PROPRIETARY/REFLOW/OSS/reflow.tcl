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
	# sanitize arguments; here you a sample of the magnitude of stupidity
	# I have to fight when glueing together Windows and Cygwin stuff;
	# the last argument (sometimes!) has a CR character appended at the
	# end, and you wouldn't believe how much havoc that can cause
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

set ST(VER) 0.01

## double exit avoidance flag
set DEAF 0

###############################################################################

array set STNAMES {
			"D"	"disconnected"
			"H"	"connecting"
			"I"	"idle"
			"R"	"running"
}

# rc file #####################################################################

set PM(RCF) ".rflowrc"
if { [info exists env(HOME)] && $env(HOME) != "" } {
	set e $env(HOME)
} else {
	set e [pwd]
}

set PM(RCF) [file join $e $PM(RCF)]
unset e

###############################################################################

# maximum packet length
set PM(MPL) 106

# UART rate
set PM(DSP) 115200

# maximum number of log lines
set PM(MXL) 1024

# Minimum and maximum temperature + span
set PM(MNT) 25
set PM(MXT) 300
set PM(SPT) [expr $PM(MXT) - $PM(MNT)]

###############################################################################

set PROF(PR) {
	{ CSpan { 0 1024 } }
	{ Integrator { 0 1024 } }
	{ Differentiator { 0 1024 } }
}

set PROF(MX) 24

set PROF(EN) [list [list 0 1000] [list $PM(MNT) $PM(MXT)]]

set PROF(DEFAULT) { "DEFAULT" { 200 10 5 } {
					{   0 150.0 }
					{ 101 183.0 }
					{  88 220.0 }
					{  37 183.0 }
					{  19 150.0 }
					{   0  80.0 }
		} }

###############################################################################

# log window, log font, log, and the number of lines
set WN(LOG) ""
set FO(LOG) "-family courier -size 9"
set ST(LOG) ""
set ST(LOL) 0

# Perceived board status: D-disconnected, H-handshake, I-idle, R-running,
# C-collecting
set ST(AST) "?"

# Debug flag
set ST(DBG) 0

# Redraw requests
set ST(RDR,XA) 0
set ST(RDR,YA) 0
set ST(RDR,PR) 0
set ST(RDR,RU) 0

# Modification flag for current profile
set ST(MOD) 0

# Current segment for menu command
set ST(CSE) ""

# Current coordinates for menu command
set ST(CCO) ""

# Hover label to show segment parameters
set ST(HOV) ""

###############################################################################

# initial graph canvas width and height
set WN(GRW) 640
set WN(GRH) 512

# graph canvas margins
set WN(LMA) 8
set WN(RMA) 8
set WN(BMA) 8
set WN(UMA) 8

# label offset from the mouse point
set WN(LOF) 5

# object distance in pixels to consider close
set WN(PRX) 6

# small font for axis labels
set FO(SMA) "TkSmallCaptionFont"

# fixed font
set FO(FIX) $FO(LOG)

# axis color
set WN(AXC) "#0000FF"

# profile normal collor
set WN(PNC) "green"

# profile lag color
set WN(PLC) "lightgray"

# profile point color
set WN(PPC) "#0FFFF0"

# run color
set WN(RNC) "#FF0000"

# color of the dashed line indicating point coordinates
set WN(DLC) "#880088"

# color for backgrounds of special labels
set WN(SLB) "gray"

# axis background color
set WN(ABC) "gray"

# state label foreground color
set WN(SLC) "red"

# foreground color for profile name label
set WN(PNL) "blue"

# current temperature thermometer color
set WN(CTC) "red"

# target temperature indicator color
set WN(TTC) "green"

# profile point radius
set WN(PPR) 6

# runpoint radius
set WN(RPR) 2

# y-tick width
set WN(YTS) 4

# x-tick width
set WN(XTS) 4

# width of the vertical axis canvas
set WN(VAW) 25

# height of the horizontal axis canvas
set WN(HAH) $WN(VAW)

# width of the thermometer canvas
set WN(TEW) 10

# Current state name (as displayed in root window)
set WN(STL) "???"

# Time (second in a cycle)
set WN(TIM) ""

# Current temperature
set WN(CUT) ""

# Target temperature
set WN(TAT) ""

# Profile name
set WN(PNA) ""

# UART descriptor
set ST(SFD) ""

# UART device
set ST(UDV) ""

# Waiting for reflow report
set ST(WRU) 0

# Ignore count for oven state update, to prevent jumps after manual change
set ST(OSL) 0

# Maximum number of stray heartbeats before the first reflow report
set PM(MXH) 3

# Emulation flag
set WN(EMU) 0

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

package require unames

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

package provide boss 1.0

###############################################################################
# BOSS ########################################################################
###############################################################################

namespace eval BOSS {

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

# the ACK flag
set B(ACK) [expr 0x04]

# direct packet type
set B(MAD) [expr 0xAC]

# acked packet type
set B(MAP) [expr 0xAD]

# preamble byte
set B(IPR) [format %c [expr 0x55]]

# diag preamble (for packet modes) = ASCII DLE
set B(DPR) [format %c [expr 0x10]]

# output busy flag (current message pending) + output event variable; also
# used as main event trigger for all vital events
set B(OBS) 0

# output queue
set B(OQU) ""

# reception automaton state
set B(STA) 0

# reception automaton remaining byte count
set B(CNT) 0

# send callback
set B(SCB) ""

# long retransmit interval
set B(RTL) 2048

# short retransmit interval
set B(RTS) 250

# function to call on packet reception
set B(DFN) "bo_nop"

# function to call on UART close (which can happen asynchronously)
set B(UCF) ""

# low-level reception timer
set B(TIM)  ""

# packet timeout (msec), once reception has started
set B(PKT) 80

###############################################################################

proc bo_nop { args } { }

proc boss_chks { wa } {

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

proc bo_abort { msg } {

	variable B

	if { $B(ABR) != "" } {
		$B(ABR) $msg
		exit 1
	}

	catch { puts stderr $msg }
	exit 1
}

proc bo_diag { } {

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

	return "::BOSS::$fun"
}

proc bo_emu_readable { fun } {
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

	set B(ONR) [after $B(ROT) "[lize bo_emu_readable] $fun"]
}

proc bo_fnwrite { msg } {
#
# Writes a packet to the UART
#
	variable B

	set ln [string length $msg]
	if { $ln > $B(MPL) } {
		bo_abort "assert fnwrite: $ln > max ($B(MPL))"
	}

	if { $ln < 2 } {
		bo_abort "assert fnwrite: length ($ln) < 2"
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
				[boss_chks $msg]]"
		flush $B(SFD)
	}] {
		boss_close "BOSS write error"
	}
}

proc bo_send { } {
#
# Callback for sending packets out
#
	variable B
	
	# cancel the callback, in case called explicitly
	if { $B(SCB) != "" } {
		catch { after cancel $B(SCB) }
		set B(SCB) ""
	}

	if !$B(OBS) {
		# just in case, this is a consistency invariant
		set B(OUT) ""
	}

	set len [string length $B(OUT)]
	# if len is nonzero, an outgoing message is pending; its length
	# has been checked already (and is <= MAXPL)

	set flg [expr $B(CUR) | ( $B(EXP) << 1 )]

	if { $len == 0 } {
		# ACK only
		set flg [expr $flg | $B(ACK)]
	}

	bo_fnwrite "[binary format cc $B(MAP) $flg]$B(OUT)"

	set B(SCB) [after $B(RTL) [lize bo_send]]
}

proc bo_write { msg { byp 0 } } {
#
# Write out a message
#
	variable B

	set lm [expr $B(MPL) - 2]

	if { [string length $msg] > $lm } {
		# truncate the message to size, probably a bad idea
		set msg [string range $msg 0 [expr $lm - 1]]
	}

	if $byp {
		# immediate output, direct protocol
		bo_fnwrite "[binary format cc $B(MAD) 0]$msg"
		return
	}

	if $B(OBS) {
		bo_abort "assert write: output busy"
	}

	set B(OUT) $msg
	set B(OBS) 1

	bo_send
}

proc bo_timeout { } {

	variable B

	if { $B(TIM) != "" } {
		bo_rawread 1
		set B(TIM) ""
	}
}

proc bo_rawread { { tm 0 } } {
#
# Called whenever data is available on the UART (mode S)
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
					# something has started, set up timer
					# if not running already
					if { $B(TIM) == "" } {
						set B(TIM) \
				            	    [after $B(PKT) \
							[lize bo_timeout]]
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
				bo_receive
				continue
			}

			# merged packets
			append B(BUF) [string range $chunk 0 [expr $B(CNT) - 1]]
			set chunk [string range $chunk $B(CNT) end]
			bo_receive
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
			bo_diag
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
			bo_diag
		}

		default {
			set B(STA) 0
		}
		}
	}
}

proc bo_receive { } {
#
# Handle a received packet
#
	variable B
	
	# dmp "RCV" $B(BUF)

	# validate CRC
	if [boss_chks $B(BUF)] {
		return
	}

	# strip off the checksum
	set msg [string range $B(BUF) 0 end-2]
	set len [string length $msg]

	if { $len < 2 } {
		# ignore it
		return
	}

	# extract the header
	binary scan $msg cucu pr fg

	# trim the message
	set msg [string range $msg 2 end]

	if { $pr == $B(MAD) } {
		# direct, receive right away, nothing else to do
		$B(DFN) $msg
		# count received messages as a form of heartbeat
		return
	}

	if { $pr != $B(MAP) } {
		# wrong magic
		return
	}

	set cu [expr $fg & 1]
	set ex [expr ($fg & 2) >> 1]
	set ac [expr $fg & 4]

	if $B(OBS) {
		# we have an outgoing message
		if { $ex != $B(CUR) } {
			# expected != our current, we are done with this
			# packet
			set B(OUT) ""
			set B(CUR) $ex
			set B(OBS) 0
		}
	} else {
		# no outgoing message, set current to expected
		set B(CUR) $ex
	}

	if $ac {
		# treat as pure ACK and ignore
		return
	}

	if { $cu != $B(EXP) } {
		# not what we expect, speed up the NAK
		catch { after cancel $B(SCB) }
		set B(SCB) [after $B(RTS) [lize bo_send]]
		return
	}

	# receive it
	$B(DFN) $msg

	# update expected
	set B(EXP) [expr 1 - $B(EXP)]

	# force an ACK
	bo_send
}

proc boss_reset { } {
#
# Protocol reset
#
	variable B

	# queue of outgoing messages
	set B(OQU) ""

	# current outgoing message
	set B(OUT) ""

	# output busy flag
	set B(OBS) 0

	# expected
	set B(EXP) 0

	# current
	set B(CUR) 0

	if { $B(SCB) != "" } {
		# abort the callback
		catch { after cancel $B(SCB) }
		set B(SCB) ""
	}
}

proc boss_init { ufd mpl { clo "" } { emu 0 } } {
#
# Initialize: 
#
#	ufd - UART descriptor
#	mpl - max packet length
#	clo - function to call on UART close (can happen asynchronously)
#	emu - emulate 'readable'
#
	variable B

	boss_reset

	set B(STA) 0
	set B(CNT) 0
	set B(BUF) ""

	set B(SFD) $ufd
	set B(MPL) $mpl

	set B(UCF) $clo

	# User packet length: in the S mode, the Network ID field is used by
	# the protocol
	set B(UPL) [expr $B(MPL) - 2]

	fconfigure $B(SFD) -buffering full -translation binary

	# start the write callback for the persistent stream
	bo_send

	# insert the auto-input handler
	if $emu {
		# the readable flag doesn't work for UART on some Cygwin
		# setups
		set B(ROT) 1
		set B(RMX) $emu
		bo_emu_readable "[lize bo_rawread]"
	} else {
		# do it the easy way
		fileevent $B(SFD) readable "[lize bo_rawread]"
	}
}

proc boss_oninput { { fun "" } } {
#
# Declares a function to be called when a packet is received
#
	variable B

	if { $fun == "" } {
		set fun "bo_nop"
	} else {
		set fun [gize $fun]
	}

	set B(DFN) $fun
}

proc boss_trigger { } {
	set ::BOSS::B(OBS) $::BOSS::B(OBS)
}

proc boss_wait { } {

	variable B

	if !$B(OBS) {
		# not busy
		if { $B(OQU) != "" } {
			# something in queue
			if { $B(SFD) != "" } {
				bo_write [lindex $B(OQU) 0]
				set B(OQU) [lrange $B(OQU) 1 end]
			} else {
				# drop everything, this is just a precaution
				set B(OQU) ""
			}
		}
	}

	vwait ::BOSS::B(OBS)
}

proc boss_stop { } {
#
# Stop the protocol
#
	variable B

	foreach cb { "SCB" "TIM" } {
		# kill callbacks
		if { $B($cb) != "" } {
			catch { after cancel $B($cb) }
			set B($cb) ""
		}
	}

	boss_reset

	set B(STA) 0
	set B(CNT) 0
	set B(BUF) ""
}

proc boss_close { { err "" } } {
#
# Close the UART (externally or internally, which can happen asynchronously)
#
	variable B

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
	boss_stop

	boss_oninput

	boss_trigger
}

proc boss_send { buf { urg 0 } } {
#
# This is the user-level output function
#
	variable B

	if { $B(SFD) == "" } {
		# ignore if disconnected, this shouldn't happen
		return
	}

	# dmp "SND<$urg>" $buf

	if $urg {
		bo_write $buf 1
		return
	}

	lappend B(OQU) $buf

	boss_trigger
}

proc boss_timeouts { slow { fast 0 } } {
#
# Sets the timeouts:
#
#	slow	periodic ACKS
#	fast	nack after unexpected packet
#
	variable B

	if { $slow == 0 } {
		set slow 1000000000
	}
	set B(RTL) $slow
	if $fast {
		set B(RTS) $fast
	}

	if { $B(SCB) != "" } {
		bo_send
	}
}

namespace export boss_*

}

namespace import ::BOSS::*

###############################################################################
# End of BOSS #################################################################
###############################################################################

package require boss

proc trigger { } { boss_trigger }

proc event_loop { } { boss_wait }

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
package require autoconnect

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

proc terminate { } {

	global DEAF ST

	if $DEAF { return }

	if $ST(MOD) {
		if { ![catch { confirm "Current profile not saved.\
				Do you really want to quit?" } er] && !$er } {
			return
		}
	}

	set DEAF 1

	# save parameters
	setsparams

	if [isstate "R"] {
		# we are running, so abort the cycle
		send_stop 1
	}

	exit 0
}

proc isinteger { u } {

	if [catch { expr $u } v] {
		return 0
	}

	if [regexp -nocase "\[.e\]" $u] {
		return 0
	}

	return 1
}

proc valid_int_number { n min max } {

	if ![isinteger $n] {
		return 0
	}

	if { $n < $min || $n > $max } {
		return 0
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

proc valid_profile_name { nam } {

	if { $nam == "" } {
		return "profile name is empty"
	}

	if ![regexp -nocase "^\[a-z\]\[a-z0-9_\]*$" $nam] {
		return "profile name ($nam) is illegal, must start with a\
			letter and be alphanumeric"
	}

	return ""
}

proc valid_profile { pr } {
#
# Validates a profile, returns "" or error code
#
	global PROF

	lassign $pr nam par pro

	set er [valid_profile_name $nam]

	if { $er != "" } {
		return $er
	}

	set i 1
	foreach k $PROF(PR) v $par {
		# the gain parameters
		set n [lindex $k 0]
		set min [lindex $k 1 0]
		set max [lindex $k 1 1]
		if ![valid_int_number $v $min $max] {
			return "profile $nam, gain parameter number $i ($n) in\
				profile $nam is invalid ($v),\
				the range is $min-$max"
		}
		incr i
	}

	set k [llength $pro]
	if { $k > $PROF(MX) } {
		return "the list of segments in profile $nam is too long\
			($k items), the maximum length is $PROF(MX) items"
	}

	set i 1
	foreach k $pro {
		lassign $k sec tem
		lassign [lindex $PROF(EN) 0] min max
		if ![valid_int_number $sec $min $max] {
			return "duration in segment $i in profile $nam is\
				invalid ($sec), the range is $min-$max"
		}
		lassign [lindex $PROF(EN) 1] min max
		if ![valid_fp_number $tem $min $max 1] {
			return "temperature in segment $i in profile $nam is\
				invalid ($tem), the range is $min-$max"
		}
	}

	return ""
}

proc add_def_profile { } {
#
# Make sure there is a default profile as a first item on the list
#
	global PM PROF

	set ni [lsearch -exact -index 0 $PM(PRL) "DEFAULT"]

	if { $ni == 0 } {
		# things are fine
		return
	}

	if { $ni > 0 } {
		set dp [lindex $PM(PRL) $ni]
		set PM(PRL) [lreplace $PM(PRL) $ni $ni]
	} else {
		set dp $PROF(DEFAULT)
	}

	set PM(PRL) [concat [list $dp] $PM(PRL)]
	log "adding built-in default profile"
}

proc getsparams { } {
#
# Retrieve parameters from RC file
#
	global PM PROF

	set pl ""

	while 1 {

		if { $PM(RCF) == "" } {
			# switched off
			log "RC params switched off"
			break
		}

		if [catch { open $PM(RCF) "r" } fd] {
			# no such file
			log "can't open RC params file, $PM(RCF), $fd"
			break
		}

		if [catch { read $fd } pars] {
			catch { close $fd }
			log "can't read RC params file, $PM(RCF), $pars"
			break
		}
		catch { close $fd }

		foreach t $pars {
			if { [lindex $t 0] == "PROFILES" } {
				set pl [lindex $t 1]
				break
			}
		}

		break
	}

	# validate the list of profiles
	set rp ""

	set np 0
	set nv 0
	foreach t $pl {
		if { [valid_profile $t] != "" } {
			incr nv
			continue
		}
		set nam [lindex $t 0]
		if [info exists pnames($nam)] {
			# ignore duplicates
			incr nv
			continue
		}
		lappend rp $t
		set pnames($nam) ""
		incr np
	}

	set msg "$np valid profiles in RC file"
	if $nv {
		append msg ", $nv invalid profiles"
	}
	log $msg

	set PM(PRL) $rp

	add_def_profile
}

proc setsparams { } {
#
# Save parameters in RC file
#
	global PM

	if { $PM(RCF) == "" } {
		# disabled
		return
	}

	if [catch { open $PM(RCF) "w" } fd] {
		alert "Cannot save parameters in $PM(RCF), failed to open: $fd"
		set PM(RCF) ""
		return
	}

	if [catch {
		puts -nonewline $fd [list [list "PROFILES" $PM(PRL)]]
	} err] {
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

	set line "[clock format [clock seconds] -format %H:%M:%S]: $line"

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
}

proc log_open { } {
#
# Opens the log window
#
	global ST WN FO

	if { $WN(LOG) != "" } {
		# already open
		return
	}

	set w ".logw"
	toplevel $w
	wm title $w "Log"

	set c $w.log

	set WN(LOG) [text $c -width 80 -height 24 -borderwidth 2 \
		-setgrid true -wrap none \
		-yscrollcommand "$w.scrolly set" \
		-xscrollcommand "$w.scrollx set" \
		-font $FO(LOG) \
		-exportselection 1 -state normal]

	scrollbar $w.scrolly -command "$c yview"
	scrollbar $w.scrollx -orient horizontal -command "$c xview"

	pack $w.scrolly -side right -fill y
	pack $w.scrollx -side bottom -fill x
	pack $c -side top -expand y -fill both

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

	$WN(LOB) configure -text "Log Off"
}

proc log_close { } {

	global WN

	catch { destroy .logw }
	set WN(LOG) ""
	$WN(LOB) configure -text "Log On"
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

###############################################################################

proc trc { msg } {

	global ST

	if !$ST(DBG) {
		return
	}

	puts $msg
}

proc dmp { hdr buf } {

	global ST

	if !$ST(DBG) {
		return
	}

	set code ""
	set nb [string length $buf]
	binary scan $buf cu$nb code
	set ol ""
	foreach co $code {
		append ol [format " %02x" $co]
	}
	trc "$hdr:$ol"
}

proc update_state { args } {

	global ST WN STNAMES

	# args == exceptional states, i.e., don't move out of them
	if { [lsearch -exact $args $ST(AST)] < 0 } {
		set s [lindex $args 0]
		if { $s != "" } {
			# trc "STATE -> $s"
			set ST(AST) $s
			if [info exists STNAMES($s)] {
				set WN(STL) $STNAMES($s)
			} else {
				# impossible
				set WN(STL) "???"
			}
			update_widgets
		}
	}
	# do trigger unconditionally, so update_state at least generates an
	# event, in fact update_state (no args) is equivalent to trigger
	trigger
}

proc isstate { args } {

	global ST

	if { [lsearch -exact $args $ST(AST)] >= 0 } {
		return 1
	}

	return 0
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
	    -blocking 0 -eofchar "" -ttycontrol { RTS 0 } } \
	      err] {
		catch { close $ST(SFD) }
		# trc "fconfig failed: $err"
		set ST(SFD) ""
		return 0
	}

	set ST(UDV) $d

	# configure the protocol
	boss_init $ST(SFD) $PM(MPL) uart_close $emu

	boss_oninput uart_read

	return 1
}

proc uart_close { { err "" } } {
#
# Called from boss_close
#
	global ST

	# trc "close UART: $err"

	if ![isstate "D" "H"] {
		if [isstate "R"] {
			# send stop in bypass mode
			send_stop 1
		}
		if { $err == "" } {
			set err "drop"
		}
		log "connection closed on $ST(UDV) <$err>"
	}
	set ST(SFD) ""
	update_state "D"
}

proc uart_connected { } {

	global ST

	return [expr { $ST(SFD) != "" }]
}

proc pack2 { val } {

	return [binary format cc [expr  $val        & 0xff] \
				 [expr ($val >>  8) & 0xFF] \
	       ]
}

proc pack1 { val } {

	return [binary format c [expr  $val & 0xff]]
}

proc send_stop { { urgent 0 } } {

	set msg [binary format c 0x02]
	log "-> stop"
	boss_send $msg $urgent
	if $urgent {
		# send in triplicate if unreliable
		boss_send $msg $urgent
		boss_send $msg $urgent
	}
}

proc send_emulation { v } {

	log "-> emul <$v>"
	boss_send "[binary format cc 0x04 $v]"
}

proc send_oven { v } {

	log "-> oven $v"
	boss_send "[binary format s 0x0303][pack2 $v]"
}

proc send_handshake { } {

	update_state "H"

	boss_send [binary format c 0xFF]
	boss_send [binary format c 0xFE]
	boss_send [binary format c 0x02]
}

proc send_start { } {
#
# Send a message to start a new cycle
#
	global WN ST

	log "-> start"

	set msg [binary format cc 0x01 $ST(NS)]

	append msg [pack2 $WN(PAS)]
	append msg [pack2 $WN(PAI)]
	append msg [pack2 $WN(PAD)]

	for { set i 0 } { $i < $ST(NS) } { incr i } {
		set dur $ST(SD,$i)
		set tem [expr round ($ST(SM,$i) * 10.0)]
		append msg [pack2 $dur]
		append msg [pack2 $tem]
	}

	boss_send $msg
}

###############################################################################

proc reflow { } {
#
# Start a reflow run
#
	global ST

	clear_run
	do_trimx
	send_start
	update_state "R"

	# Waiting for report
	set ST(WRU) 1
}

proc run_button { } {
#
	global WN ST

	if [isstate "I"] {
		log "starting reflow cycle ..."
		reflow
	} elseif [isstate "R"] {
		log "aborting reflow cycle ..."
		send_stop
	}
}

###############################################################################

proc uart_read { msg } {

	global ST PM WN

	# dmp "RCV" $msg
	set len [string length $msg]

	if { $len == 8 } {
		# this can only be a heartbeat
		binary scan $msg cucusususu a em b tem ovn
		if { $a != 0x7C || $b != 0x7A31 } {
			# trc "BAD MAGIC!"
			return
		}
		autocn_heartbeat
		# maintain the emulation status
		if $em {
			set WN(EMU) 1
		} else {
			set WN(EMU) 0
		}
		if [isstate "H"] {
			# waiting for handshake
			log "handshake established with $ST(UDV)"
			update_state "I"
		} elseif [isstate "R"] {
			if $ST(WRU) {
				# waiting for the first report
				if { $ST(WRU) > $PM(MXH) } {
					# assume we have failed to start
					log "failed to start cycle"
					update_state "I"
					set ST(WRU) 0
				} else {
					log "spurious heartbeat <$ST(WRU)>"
					incr ST(WRU)
				}
			} else {
				# assume cycle completed
				log "reflow cycle ends"
				update_state "I"
			}
		}

		# update temperature and oven status
		set tem [fmtr $tem]
		temp_report $tem CUT
		oven_report $ovn
		return
	}

	if { $len != 12 } {
		# resync
		# trc "MSG ignored"
		return
	}

	binary scan $msg cucusususususu a em sec tem ovn seg tar

	if { $a != 0x01 } {
		# trc "BAD report signature"
		return
	}
	autocn_heartbeat
	if $em {
		set WN(EMU) 1
	} else {
		set WN(EMU) 0
	}
	if ![isstate "R"] {
		# stray report
		log "stray report"
		send_stop
		temp_report [fmtr $tem] CUT
		oven_report $ovn
		return
	}

	set ST(WRU) 0
	new_sample $sec $tem $ovn $seg $tar
}

proc handshake_ok { } {

	return [isstate "I" "R"]
}

###############################################################################
	
proc fmtt { t } {
#
# Format a temperature value
#
	return [format %1.1f $t]
}

proc fmtr { t } {
#
# Format a raw temperature value arriving from the node, i.e., an integer
# number of tenths of degree
#
	return [format %1.1f [expr double($t) / 10.0]]
}

proc new_sample { sec tem ovs seg tar } {
#
# New sample from the controller
#
	global ST WN PM

	set cu [fmtr $tem]
	set ta [fmtr $tar]

	# a few sanity checks
	if { $WN(TIM) == "" && $sec > 20 ||
	     $WN(TIM) != "" && $sec > [expr $WN(TIM) + 20] } {
		# trc "error in report, sec $sec"
		return
	}

	if { $cu > $PM(MXT) || $ta > $PM(MXT) || $ovs > 1024 } {
		# trc "error in report, temp/oven $cu $ta $ovs"
		return
	}

	temp_report $cu CUT
	temp_report $ta TAT
	oven_report $ovs

	update_run $sec $seg $cu

	log \
	". [format %4d $sec], T: [format %5.1f $cu] --> [format %5.1f $ta],\
	 O: [format %4d $ovs], G: [format %2d $seg]"
}

proc temp_report { val where } {

	global WN PM

	if { $val > $PM(MXT) } {
		return
	}

	if { $WN($where) != $val } {
		set WN($where) $val
		redraw_temp $where
	}
}

proc oven_report { val } {

	global WN ST

	if { $ST(OSL) < 0 } {
		# locked
		return
	}

	if $ST(OSL) {
		incr ST(OSL) -1
		return
	}

	if [valid_int_number $val 0 1024] {
		set WN(OVS) $val
	}
}

proc mk_rootwin { win } {

	global WN ST PM FO

	wm title $win "Olsonet Reflow Controller V$ST(VER)"

	if { $win == "." } {
		set w ""
	} else {
		set w $win
	}

	#######################################################################

	set w [frame $w.f]
	pack $w -side top -padx 5 -pady 5 -expand y -fill both

	set r 0
	set c 0

	# top label frame above the graph canvas
	set f [frame $w.f_${r}_${c}]
	grid $f -column $c -row $r -sticky news
	incr c
		# top labels: state and current time
		label $f.sh -text "State: " -anchor w
		pack $f.sh -side left
		label $f.sl -textvariable WN(STL) -anchor w -bg $WN(SLB) \
			-fg $WN(SLC)
		pack $f.sl -side left
		label $f.tl -textvariable WN(TIM) -anchor e -width 6 \
			-bg $WN(SLB)
		pack $f.tl -side right
		label $f.th -text "Time: " -anchor w
		pack $f.th -side right

		label $f.gap -text "      "
		pack $f.gap -side right
		set WN(CEM) [checkbutton $f.emb -variable WN(EMU) \
			-command do_emulation]
		pack $f.emb -side right
		label $f.eml -text "Emulation:"
		pack $f.eml -side right

		set tip "Shows the controller state which can be:\
			 disconnected (no device found), connecting (trying\
			 to identify the device), idle (connected),\
			 running (executing a reflow cycle)."

		tip_set $f.sh $tip
		tip_set $f.sl $tip

		set tip "Shows the time (second) of the reflow cycle in\
			 progress; blank when no cycle is being executed."

		tip_set $f.tl $tip
		tip_set $f.th $tip

		set tip "Check this box to run a cycle in the emulation mode.\
			 Note that the emulation mode is implemented in the\
			 device, so the device must be connected."

		tip_set $f.eml $tip
		tip_set $f.emb $tip

	# dummy frame above the first vertical axis
	set f [frame $w.f_${r}_${c}]
	grid $f -column $c -row $r -sticky news
	incr c
	
	# label frame above the thermometer canvas
	set f [frame $w.f_${r}_${c}]
	grid $f -column $c -row $r -sticky news
	incr c
		# thermometer label
		label $f.th -text "CT" -anchor center
		pack $f.th -side top

		set tip_ct "Current oven temperature (in degrees centigrade)\
			    as reported by the device."

		tip_set $f.th $tip_ct

	# dummy frame above the second vertical axis
	set f [frame $w.f_${r}_${c}]
	grid $f -column $c -row $r -sticky news
	incr c

	# label frame above the target temperature canvas
	set f [frame $w.f_${r}_${c}]
	grid $f -column $c -row $r -sticky news
	incr c
		# target tmp label
		label $f.th -text "TT" -anchor center
		pack $f.th -side top

		set tip_ta "Target temperature (in degrees centigrade) at the\
			    current stage of the reflow cycle; only shown when\
			    running a cycle."

		tip_set $f.th $tip_ta

	#######################################################################

	incr r
	set c 0

	# the graph canvas
	set gphcol $c
	set gphrow $r
	set WN(GRP) $w.gr
	set he [expr $WN(GRH) + $WN(BMA) + $WN(UMA)]
	set wi [expr $WN(GRW) + $WN(LMA) + $WN(RMA)]
	canvas $WN(GRP) -width $wi -height $he -bg black
	grid $WN(GRP) -column $c -row $r -sticky news
	# resize offsets: how much the resized (event) parameters differ from
	# the actual (specified) size (to be learned at first event)
	set WN(GRP,G) ""
	bind $WN(GRP) <Configure> "do_resize GRP %w %h"
	bind $WN(GRP) <ButtonRelease-3> "menu_pp %x %y %X %Y"
	bind $WN(GRP) <ButtonPress-1> "show_lines %x %y"
	bind $WN(GRP) <ButtonRelease-1> "move_leave"
	incr c

	# the vertical axis canvas
	set WN(VA0) $w.va0
	canvas $WN(VA0) -width $WN(VAW) -height $he -bg $WN(ABC)
	grid $WN(VA0) -column $c -row $r -sticky ns
	set WN(VA0,G) ""
	bind $WN(VA0) <Configure> "do_resize VA0 %w %h"
	incr c

	# the thermometer canvas
	set WN(THM) $w.th
	canvas $WN(THM) -width $WN(TEW) -height $he -bg black
	grid $WN(THM) -column $c -row $r -sticky ns
	tip_set $WN(THM) $tip_ct
	incr c

	# the second axis canvas
	set WN(VA1) $w.va1
	canvas $WN(VA1) -width $WN(VAW) -height $he -bg $WN(ABC)
	grid $WN(VA1) -column $c -row $r -sticky ns
	set WN(VA1,G) ""
	bind $WN(VA1) <Configure> "do_resize VA1 %w %h"
	incr c

	# the target temperature canvas
	set WN(THT) $w.tt
	canvas $WN(THT) -width $WN(TEW) -height $he -bg black
	grid $WN(THT) -column $c -row $r -sticky ns
	tip_set $WN(THT) $tip_ta
	set maxcol $c
	incr c

	#######################################################################

	incr r
	set c 0

	# the bottom axis canvas
	set WN(HA0) $w.ha0
	canvas $WN(HA0) -width $wi -height $WN(HAH) -bg $WN(ABC)
	grid $WN(HA0) -column $c -row $r -sticky we
	set WN(HA0,G) ""
	bind $WN(HA0) <Configure> "do_resize HA0 %w %h"
	incr c

	# this grid slot is empty: rectangle below the first vertical axis
	incr c

	# the rectangle underneath the thermometer; make it into a label
	# showing current temperature
	set WN(CUT) [fmtt $PM(MNT)]
	set WN(TAT) $WN(CUT)
	label $w.thb -textvariable WN(CUT) -anchor center -bg $WN(CTC) -width 6
	grid $w.thb -column $c -row $r -sticky news
	tip_set $w.thb $tip_ct

	incr c

	# another empty slot (underneath the second vertical axis)
	incr c

	# one more rectangle under the target temperature indicator
	label $w.ttb -textvariable WN(TAT) -anchor center -bg $WN(TTC) -width 6
	grid $w.ttb -column $c -row $r -sticky news
	tip_set $w.ttb $tip_ta

	set maxrow $r

	#######################################################################
	#######################################################################

	set bf $w.bof

	frame $bf
	incr r
	set maxrow $r

	grid $bf -columnspan [expr $maxcol + 1] -row $r -sticky news

	#######################################################################

	set z $bf

	# sliders for the three parameters of the control algorithm
	set f [labelframe $z.pf -text "Controller"]
	pack $f -side left -expand y -fill both

	#######################################################################

	set row 0
	foreach p { "S" "I" "D" } l { 0 0 0 } {
		label $f.l$p -text " $p: " -anchor w -font $FO(FIX)
		grid $f.l$p -column 0 -row $row -sticky w
		set WN(PA$p,M) ""
		set WN(WC$p) [scale $f.s$p -orient horizontal \
			-from $l -to 1024 \
			-resolution 1 -variable WN(PA$p) -showvalue 1 \
			-command "do_setparam $p"]
		grid $f.s$p -column 1 -row $row -sticky we
		incr row
	}

	tip_set $WN(WCS) "The span parameter. Lower settings make the\
			  controller more aggressive, i.e., the oven tends\
			  to be set to a higher value for a given temperature\
			  difference."

	tip_set $WN(WCI) "Integrator gain. Higher settings tend to amplify\
			  small temperature differences by increasing the\
			  oven value if the difference persists, i.e., the\
			  oven appears to give up too early."

	tip_set $WN(WCD) "Differentiator gain. Higher settings tend to\
			  compensate for overshooting by reducing the oven\
			  value when the temperature grows too fast."


	grid columnconfigure $f 0 -weight 0
	grid columnconfigure $f 1 -weight 1

	#######################################################################

	set g [frame $z.cf]
	pack $g -side right -expand n -fill both

	#######################################################################

	set f [labelframe $g.oc -text "Oven"]
	pack $f -side top -expand y -fill x

	set WN(WOV) [scale $f.os -orient horizontal -from 0 -to 1024 \
		-resolution 1 \
		-variable WN(OVS) -showvalue 1 -command "do_updoven"]
	bind $WN(WOV) <ButtonRelease> "do_setoven"
	pack $f.os -side top -expand y -fill x

	tip_set $WN(WOV) "Manual oven controller. Available in the idle state\
			  only (device connected, no reflow cycle in progress)."

	set h [frame $g.bf]
	pack $h -side top -expand y -fill both

	#######################################################################
	
	set f [labelframe $h.qf -text "Profile"]
	pack $f -side left -expand n -fill both

	label $f.p -textvariable WN(PNA) -bg $WN(SLB) -width 20 -fg $WN(PNL)
	pack $f.p -side top -expand y -fill x

	tip_set $f.p "Shows the name of the current profile."

	set f [frame $f.b]
	pack $f -side bottom -expand n -fill x
	
	set WN(WLB) [button $f.l -text "Load" -command load_profile -width 20]
	pack $f.l -side top -expand n -fill x

	set tip_pe "Note that you can edit the current profile. Try dragging\
		    the points, also right clicking on the points, on the green\
		    lines, and anywhere inside the main graph canvas."

	tip_set $f.l "Use this button to load a different profile from your\
		      collection. $tip_pe"

	set WN(WSB) [button $f.s -text "Save" -command save_profile]
	pack $f.s -side top -expand n -fill x

	tip_set $f.s "Use this button to save the current profile, possibly\
		      under a different name. $tip_pe"

	#######################################################################

	set f [labelframe $h.rf -text "Control"]
	pack $f -side right -expand n -fill both

	#######################################################################

	set f [frame $f.b]
	pack $f -side bottom -expand y -fill x

	set WN(LOB) [button $f.l -text "Log On" -command log_toggle -width 20]
	pack $f.l -side top -expand n -fill x

	tip_set $f.l "Toggles the log window."

	set WN(WRB) [button $f.r -text "Run" -command run_button]
	pack $f.r -side top -expand n -fill x

	tip_set $f.r "Starts or aborts a reflow run. Requires a device\
		      connection."

	button $f.q -text "Quit" -command terminate
	pack $f.q -side top -expand n -fill x

	#######################################################################
	#######################################################################

	grid columnconfigure $w $gphcol \
		-minsize 300 \
		-weight 1

	for { set c 0 } { $c <= $maxcol } { incr c } {
		if { $c != $gphcol } {
			grid columnconfigure $w $c -weight 0
		}
	}

	grid rowconfigure $w $gphrow \
		-minsize 75 \
		-weight 1

	for { set r 0 } { $r <= $maxrow } { incr r } {
		if { $r != $gphrow } {
			grid rowconfigure $w $r -weight 0
		}
	}

	#######################################################################
	#######################################################################

	bind $win <Destroy> "terminate"

	#######################################################################

	set m [menu .pon -tearoff 0]
	set WN(MEN,N) $m
	gmcmd_poi $m

	set m [menu .pos -tearoff 0]
	set WN(MEN,S) $m
	gmcmd_seg $m

	set m [menu .poz -tearoff 0]
	set WN(MEN,Z) $m
	gmcmd_non $m

	set m [menu .por -tearoff 0]
	set WN(MEN,R) $m
	gmcmd_run $m

	# this one is safe for tear-off
	set m [menu .poa -tearoff 1]
	set WN(MEN,A) $m
	gmcmd_all $m
}

proc gmcmd_all { m } {
#
# Graph commands applicable in all circumstances
#
	$m add command -label "Trim x-scale" -command "do_trimx"
	$m add separator
	$m add command -label "Extra 5 seconds" -command "do_extratime 5"
	$m add command -label "Extra 10 seconds" -command "do_extratime 10"
	$m add command -label "Extra 30 seconds" -command "do_extratime 30"
	$m add command -label "Extra minute" -command "do_extratime 60"
}

proc gmcmd_nor { m } {
#
# Applicable to any non-run config
#
	$m add separator
	$m add command -label "Revert" -command "do_revert"
	$m add separator
	gmcmd_all $m
}

proc gmcmd_run { m } {
#
# Applicable to run data
#
	$m add command -label "Clear run data" -command "clear_run"
	$m add command -label "Adjust segments to run data" \
		-command "do_adjlags"
	$m add separator
	gmcmd_all $m
}

proc gmcmd_poi { m } {
#
# Applicable to point selection
#	
	$m add command -label "Delete point" -command "do_delsegment 1"
	$m add command -label "Delete segment" -command "do_delsegment 0"
	$m add command -label "Edit segment" -command "do_editsegment"
	gmcmd_nor $m
}

proc gmcmd_seg { m } {
#
# Applicable to segment selection
#
	$m add command -label "Delete segment" -command "do_delsegment 0"
	$m add command -label "Edit segment" -command "do_editsegment"
	$m add command -label "Insert point" -command "do_insertpoint"
	gmcmd_nor $m
}

proc gmcmd_non { m } {
#
# Applicable to "nothing-in-particular" selection
#
	$m add command -label "Insert point" -command "do_insertpoint"
	gmcmd_nor $m
}

###############################################################################

proc build_profile { nam } {
#
# Build a profile description from the current profile data
#
	global ST WN

	set seg ""

	for { set i 0 } { $i < $ST(NS) } { incr i } {
		lappend seg [list $ST(SD,$i) $ST(SM,$i)]
	}

	return [list $nam [list $WN(PAS) $WN(PAI) $WN(PAD)] $seg]
}

proc recalc_profile { } {
#
# Recalculates the current profile's timing
#
	global ST

	set tt 0

	for { set i 0 } { $i < $ST(NS) } { incr i } {
		incr tt $ST(SD,$i)
		# segment end time (updated when run)
		set ST(ST,$i) $tt
	}

	return $tt
}

proc reset_profile { } {
#
# Reloads the current profile
#
	global PROF WN PM ST

	set p [lindex $PM(PRL) $PROF(CUR)]

	lassign $p nam par seg

	set WN(PNA) $nam

	lassign $par WN(PAS) WN(PAI) WN(PAD)

	# mark bo run
	set WN(TIM) ""
	array unset ST "RU,*"
	array unset ST "SD,*"
	array unset ST "ST,*"
	array unset ST "SM,*"

	set ns [llength $seg]
	set ST(NS) $ns

	for { set i 0 } { $i < $ns } { incr i } {
		set sg [lindex $seg $i]
		# segment duration
		set ST(SD,$i) [lindex $sg 0]
		# target temperature
		set ST(SM,$i) [lindex $sg 1]
	}

	# total duration based on profile only; this value determines the ticks
	# on the horizontal axis; remember to redraw XA, PR, RU whenever it
	# changes
	set ST(DU) [recalc_profile]

	# redraw everything except for the vertical axis
	set ST(RDR,XA) 1
	set ST(RDR,PR) 1
	set ST(RDR,RU) 1

	# modification flag for current profile
	set ST(MOD) 0
}

proc redraw { } {

	global ST

	if $ST(RDR,XA) {
		# redraw x-axis
		redraw_xa
	}

	if $ST(RDR,YA) {
		# redraw y-axis
		redraw_ya
	}

	if $ST(RDR,PR) {
		redraw_profile
	}

	if $ST(RDR,RU) {
		redraw_run
	}
}

proc yco { y } {
#
# Mirrors the y coordinate
#
	global WN

	return [expr $WN(YTO) - $y]
}

proc xco { x } {
#
# Properly offsets the x coordinate of GRP by the margin
#
	global WN

	return [expr $WN(LMA) + $x]
}

proc ystart { } {
#
# Initialize for the y coordinate
#
	global WN

	set WN(HEI) [expr [lindex $WN(GRP,S) 1] - $WN(BMA) - $WN(UMA)]
	set WN(YTO) [expr $WN(HEI) + $WN(UMA)]

	if { $WN(HEI) < 20 } {
		# considered minimum sane height
		return 0
	}

	return $WN(HEI)
}

proc xstart { } {
#
# Initialize for the x coordinate
#
	global WN

	set WN(WID) [expr [lindex $WN(GRP,S) 0] - $WN(LMA) - $WN(RMA)]

	if { $WN(WID) < 30 } {
		# considered minimum width
		return 0
	}

	return $WN(WID)
}

proc ttoy { t } {
#
# Converts temperature to the y coordinate
#
	global PM WN

	set y [expr round((double($t - $PM(MNT)) * $WN(HEI)) / $PM(SPT))]

	if { $y < 0 } {
		return 0
	}

	if { $y > $WN(HEI) } {
		return $WN(HEI)
	}

	return $y
}

proc yctot { y } {
#
# Converts y coordinate to temperature
#
	global PM WN

	# invert it
	set y [expr $WN(YTO) - $y]
	set t [expr round((double($y * $PM(SPT)) / $WN(HEI)) + $PM(MNT))]
	if { $t < $PM(MNT) } {
		set t $PM(MNT)
	} elseif { $t > $PM(MXT) } {
		set $PM(MXT)
	}

	return $t
}

proc xctos { x } {
#
# Converts x coordinate to second
#
	global WN ST

	# from zero
	set x [expr $x - $WN(LMA)]
	set s [expr round(double($x * $ST(DU)) / $WN(WID))]

	if { $s < 0 } {
		set s 0
	} elseif { $s > $ST(DU) } {
		set s $ST(DU)
	}

	return $s
}

proc stox { t } {
#
# Converts second to the x coordinate
#
	global WN ST

	set x [expr round((double($t) * $WN(WID)) / $ST(DU))]

	if { $x < 0 } {
		return 0
	}

	if { $x > $WN(WID) } {
		return $WN(WID)
	}

	return $x
}

proc proflines { { dur "" } } {
#
# Returns the list of lines needed to draw the current profile
#
	global ST PM

	set wi [xstart]
	set hi [ystart]

	# accumulated time
	set tim 0

	if { $wi && $hi } {

		set res [list [list [xco [stox 0]] [yco [ttoy $PM(MNT)]] 0]]

		for { set i 0 } { $i < $ST(NS) } { incr i } {

			# normal target time for the segment
			set ts [expr $tim + $ST(SD,$i)]

			# updated end time
			set uts $ST(ST,$i)

			# target temperature
			set tt $ST(SM,$i)

			set nx [xco [stox $ts]]
			set ny [yco [ttoy $tt]]

			lappend res [list $nx $ny $i]

			if { $uts > $ts } {
				# lagging
				set cx [xco [stox $uts]]
				lappend res [list $cx $ny]
				set tim $uts
			} else {
				set tim $ts
			}
		}
	}

	if { $dur != "" } {
		upvar $dur dd
		set dd $tim
	}

	return $res
}

proc redraw_profile { } {
#
	global ST WN PM

	# mark as redrawn
	set ST(RDR,PR) 0

	set c $WN(GRP)
	$c delete "p"

	# this also does xstart/ystart
	set ll [proflines]

	if { $ll == "" } {
		return
	}

	lassign [lindex $ll 0] lx ly sn
	set ll [lrange $ll 1 end]

	foreach m $ll {

		lassign $m nx ny sn

		if { $sn == "" } {
			# color for extension
			$c create line $lx $ly $nx $ny -fill $WN(PLC) \
				-dash { 3 3 } -width 3 -tags "p"
		} else {
			# normal color
			$c create line $lx $ly $nx $ny -fill $WN(PNC) \
				-width 3 -tags "p"
		}


		set lx $nx
		set ly $ny
	}

	# do the points separately, so they are always on top of the lines

	foreach m $ll {

		lassign $m nx ny sn

		if { $sn != "" } {
			# normal line ending a segment, put a point at the end
			set po [$c create oval [expr $nx - $WN(PPR)] \
					       [expr $ny - $WN(PPR)] \
					       [expr $nx + $WN(PPR)] \
					       [expr $ny + $WN(PPR)] \
					       -fill $WN(PPC) -tags "p"]

			$c bind $po <B1-Motion> "move_pp $sn $po %x %y"
			$c bind $po <ButtonRelease-1> "move_pp_end $sn"
			$c bind $po <Enter> "move_enter $sn"
			$c bind $po <Leave> "move_leave"
		}
	}
}

proc redraw_run { } {
#
	global ST WN

	# mark as redrawn
	set ST(RDR,RU) 0

	$WN(GRP) delete "r"

	if { $WN(TIM) == "" } {
		# no run
		return
	}

	set wi [xstart]
	set hi [ystart]

	if { !$wi || !$hi } {
		return
	}

	for { set i 0 } { $i <= $WN(TIM) } { incr i } {
		runpoint [xco [stox $i]] [yco [ttoy $ST(RU,$i)]]
	}
}

proc runpoint { x y } {

	global WN

	$WN(GRP) create rectangle [expr $x - $WN(RPR)] [expr $y - $WN(RPR)] \
				  [expr $x + $WN(RPR)] [expr $y + $WN(RPR)] \
				  -fill $WN(RNC) -outline "" -tags "r"
}

proc update_run { sec seg tmp } {
#
# Adds one item to the run; the arguments are extracts from the node report:
# second, profile segment number, current temperature
#
	global ST WN

	# redraw request flag
	set res 0

	# last time and last temperature
	if { $WN(TIM) == "" } {
		# starting up
		set ls -1
		set res 1
		set ST(RDR,RU) 1
	} else {
		set ls $WN(TIM)
	}

	if { $sec <= $ls || $seg >= $ST(NS) } {
		# a sanity check
		return
	}

	set ms [expr $ls + 1]

	if { $ms != $sec } {
		set mr [expr $sec - $ms]
		if { $mr == 1 } {
			log ". missing report"
		} else {
			log ". missing $mr reports"
		}
	}

	for { set i $ms } { $i <= $sec } { incr i } {
		set ST(RU,$i) $tmp
	}

	set WN(TIM) $sec

	set wi [xstart]
	set hi [ystart]

	if { !$wi || !$hi } {
		return
	}

	if { $ST(ST,$seg) < $sec } {
		# the segment is lagging behind, update this one and all the
		# segments following it
		set ST(ST,$seg) $sec
		for { set j [expr $seg + 1] } { $j < $ST(NS) } { incr j } {
			incr sec $ST(SD,$j)
			set ST(ST,$j) $sec
		}
		# have to rescale things, so a global update is in order
		set res 1
		set ST(RDR,PR) 1
		set ST(RDR,RU) 1
	}

	if { $ST(DU) < $sec } {
		# must grow the x-scale
		set ST(DU) $sec
		set res 1
		set ST(RDR,PR) 1
		set ST(RDR,RU) 1
		set ST(RDR,XA) 1
	}

	if $res {
		# drastic action needed
		redraw
		return
	}

	# we can get away with just updating the picture
	set y [yco [ttoy $tmp]]
	for { set i $ms } { $i <= $sec } { incr i } {
		runpoint [xco [stox $i]] $y
	}
}

proc redraw_xa { } {
#
# Redraw the X axis
#
	global ST WN FO

	# mark as redrawn
	set ST(RDR,XA) 0

	set c $WN(HA0)
	$c delete "a"

	set wi [xstart]

	# a precaution
	if !$wi {
		return
	}

	# pixels per second
	set dd [expr double($wi) / $ST(DU)]

	set dy 10
	while { [expr $dy * $dd] < 30 } {
		incr dy 10
	}

	$c create line [xco 0] 0 [xco $wi] 0 -fill $WN(AXC) -width 3 -tags "a"
	set tc $dy

	while 1 {

		set x [stox $tc]

		if { $x < 15 } {
			incr tc $dy
			continue
		}

		if { [expr $wi - $x] < 15 } {
			break
		}

		set x [xco $x]
		$c create line $x 0 $x $WN(XTS) -fill $WN(AXC) -tags "a"
		$c create text $x $WN(XTS) \
			-text $tc -fill $WN(AXC) \
			-font $FO(SMA) -tags "a" -anchor n
		incr tc $dy
	}
}

proc redraw_ya { } {
#
# Redraw the Y axis (e.g., following a resize
#
	global ST WN PM FO

	# mark as redrawn
	set ST(RDR,YA) 0

	set cv $WN(VA0)
	set cw $WN(VA1)
	$cv delete "a"
	$cw delete "a"

	# initialize for the y coordinate
	set hi [ystart]

	# a precaution, this should never happen
	if !$hi {
		return
	}

	# pixels per degree
	set dd [expr double($hi) / ( $PM(MXT) - $PM(MNT) )]

	set dy 10
	while { [expr $dy * $dd] < 15 } {
		# make sure that the tick increment gives at least 15 pixels
		# of separation
		incr dy 10
	}

	set ts $dy
	while { $ts <= $PM(MNT) } {
		# starting temperature
		incr ts $dy
	}

	foreach c [list $cv $cw] {

		$c create line 0 [yco 0] 0 [yco $hi] -fill $WN(AXC) \
			-width 3 -tags "a"

		set tc $ts

		while 1 {

			set y [ttoy $tc]
			if { $y < 7 } {
				# not too close to the beginning
				incr tc $dy
				continue
			}

			if { [expr $hi - $y ] < 7 } {
				break
			}

			# OK, draw the tick and the value
			set y [yco $y]
			$c create line 0 $y $WN(YTS) $y -fill $WN(AXC) -tags "a"
			# the value
			$c create text $WN(YTS) $y \
				-text [format %03d $tc] -fill $WN(AXC) \
				-font $FO(SMA) -tags "a" -anchor w
			incr tc $dy
		}
	}

	redraw_temp "CUT"
	redraw_temp "TAT"
}

proc redraw_temp { where } {
#
# Redraw any of the two thermometers
#
	global WN

	if { $where == "CUT" } {
		set c $WN(THM)
		set k $WN(CTC)
	} else {
		set c $WN(THT)
		set k $WN(TTC)
	}

	$c delete "t"

	if ![ystart] {
		return
	}

	set y [yco [ttoy $WN($where)]]

	$c create rectangle 2 $y [expr $WN(TEW) - 1] [lindex $WN(GRP,S) 1] \
		-fill $k -outline $k -state disabled -tags "t"
}

proc do_resize { wi nw nh } {
#
# Called whenever any of our canvas is resized
#
	global ST WN

	set res 0

	if { $WN($wi,G) == "" } {
		# first time, immediately after window creation
		set aw [$WN($wi) cget -width]
		set ah [$WN($wi) cget -height]
		set WN($wi,G) [list [expr $nw - $aw] [expr $nh - $ah]]
		# save the last size
		set WN($wi,S) [list $aw $ah]
		if { $wi == "GRP" } {
			# force soft resize to initialize things
			set ST(RDR,XA) 1
			set ST(RDR,YA) 1
			set ST(RDR,PR) 1
			set ST(RDR,RU) 1
			redraw
		}
		return
	}

	# not the first time, these are the properly offset new dimensions
	set nw [expr $nw - [lindex $WN($wi,G) 0]]
	set nh [expr $nh - [lindex $WN($wi,G) 1]]
	set res 0
	if { $nw != [lindex $WN($wi,S) 0] } {
		# the width has changed
		set res 1
		if { $wi == "GRP" } {
			# only this canvas triggers soft resize
			set ST(RDR,XA) 1
			set ST(RDR,PR) 1
			set ST(RDR,RU) 1
		}
	}
	if { $nh != [lindex $WN($wi,S) 1] } {
		# the height has changed
		set res 1
		if { $wi == "GRP" } {
			set ST(RDR,YA) 1
			set ST(RDR,PR) 1
			set ST(RDR,RU) 1
		}
	}
	if $res {
		set WN($wi,S) [list $nw $nh]
		$WN($wi) configure -width $nw -height $nh
		if { $wi == "GRP" } {
			redraw
		}
	}
}

proc nearby { poi x y } {
#
# Checks if a point or segment line passes close to the specified point and,
# if so, returns the segment number
#
	global WN ST PM

	set ll [proflines]

	set min 1000
	set sel ""

	lassign [lindex $ll 0] lx ly

	foreach m [lrange $ll 1 end] {

		lassign $m nx ny seg

		if { $seg != "" } {

			if $poi {
				set d [point_distance $nx $ny $x $y]
			} else {
				set d [line_distance $lx $ly $nx $ny $x $y]
			}

			if { $d < $min } {
				set min $d
				set sel $seg
			}
		}

		set lx $nx
		set ly $ny
	}

	if { $min <= $WN(PRX) } {
		return $sel
	}

	return ""
}

proc point_distance { px py x y } {
#
# Distance between points
#
	set dx [expr $px - $x]
	set dy [expr $py - $y]
	return [expr sqrt( $dx * $dx + $dy * $dy )]
}

proc line_distance { ax ay bx by cx cy } {
#
# Distance from a line segment to a point rounded to integer
#
	set num [expr ($cx-$ax)*($bx-$ax) + ($cy-$ay)*($by-$ay)]
	set den [expr ($bx-$ax)*($bx-$ax) + ($by-$ay)*($by-$ay)]

	if { $den == 0 } {
		return 1000
	}

	set r [expr double($num) / double($den)]

	if { $r >= 0.0 && $r <= 1.0 } {
		set s [expr double(($ay-$cy)*($bx-$ax)-($ax-$cx)*($by-$ay) ) / \
			$den]
		set d [expr abs($s)*sqrt($den)]

		return $d
	}

	set d1 [point_distance $ax $ay $cx $cy]
	set d2 [point_distance $bx $by $cx $cy]

	if { $d2 < $d1 } {
		return $d2
	}

	return $d1
}

proc menu_pp { x y X Y } {
#
# Triggers a popup menu inside the main graph canvas
#
	global WN ST

	# to avoid using a leftover segment from previous operation
	set ST(CSE) ""

	if { $ST(AST) == "R" } {
		set m $WN(MEN,A)
	} elseif { $WN(TIM) != "" } {
		set m $WN(MEN,R)
	} else {
		# check if there is a close point
		set p [nearby 1 $x $y]
		if { $p != "" } {
			set ST(CSE) $p
			set m $WN(MEN,N)
		} else {
			set p [nearby 0 $x $y]
			if { $p != "" } {
				set ST(CSE) $p
				set m $WN(MEN,S)
			} else {
				set m $WN(MEN,Z)
			}
		}
	}

	# current coordinates
	set ST(CCO) [list $x $y $X $Y]
	tk_popup $m $X $Y
}

proc do_extratime { sec } {
#
# Extends the x-axis by extra seconds
#
	global ST

	incr ST(DU) $sec

	set ST(RDR,PR) 1
	set ST(RDR,RU) 1
	set ST(RDR,XA) 1

	redraw
}

proc do_trimx { } {
#
# Trim the x-scale to the maximum time used
#
	global WN ST

	proflines pt

	if { $WN(TIM) != "" && $WN(TIM) > $pt } {
		set pt $WN(TIM)
	}

	if { $ST(DU) != $pt } {
		# trc "Trim: DU=$ST(DU), PT=$pt"
		set ST(DU) $pt
		set ST(RDR,PR) 1
		set ST(RDR,RU) 1
		set ST(RDR,XA) 1
		redraw
	}
}

proc do_delsegment { merge } {
#
# Delete current segment (or merge it with next one)
#
	global ST WN

	if { $ST(CSE) == "" || $WN(TIM) != "" } {
		# trc "delsegment: void"
		return
	}

	if { $ST(NS) <= 2 } {
		alert "A profile needs at least two segments, cannot delete"
		return
	}

	set j 0
	set tt 0
	set du 0
	for { set i 0 } { $i < $ST(NS) } { incr i } {
		if { $i == $ST(CSE) } {
			# skip this one
			if $merge {
				# but include its duration
				set du $ST(SD,$i)
			}
			continue
		}
		incr du $ST(SD,$i)
		set ST(SD,$j) $du
		incr tt $du
		set ST(ST,$j) $tt
		set ST(SM,$j) $ST(SM,$i)
		incr j
		set du 0
	}

	# should we decrease DU? probably not
	set ST(RDR,PR) 1
	incr ST(NS) -1
	redraw
	set ST(CSE) ""
	set ST(MOD) 1
}

proc do_insertpoint { } {
#
# Adds a new point/segment to the profile
#
	global ST WN PROF

	if { $ST(CCO) == "" ||  $WN(TIM) != "" } {
		# trc "insertpoint: void
		set ST(CCO) ""
		return
	}

	if { $ST(NS) >= $PROF(MX) } {
		alert "A profile cannot have more than $PROF(MX) segments,\
			cannot insert a new point"
		set ST(CCO) ""
		return
	}

	lassign $ST(CCO) x y

	if { ![xstart] || ![ystart] } {
		# impossible
		set ST(CCO) ""
		return
	}

	# second and temperature
	set s [xctos $x]
	set t [yctot $y]

	set j 0
	set tt 0

	for { set i 0 } { $i < $ST(NS) } { incr i } {
		set tu [expr $tt + $ST(SD,$i)]
		if { $tu > $s } {
			# have to insert before this one
			break
		}
		set tt $tu
	}

	# duration of the new segment
	set d [expr $s - $tt]

	if { $i < $ST(NS) } {
		# stolen from the current segment's duration
		incr ST(SD,$i) -$d
		for { set j $ST(NS) } { $j > $i } { incr j -1 } {
			set k [expr $j - 1]
			set ST(SD,$j) $ST(SD,$k)
			set ST(SM,$j) $ST(SM,$k)
			set ST(ST,$j) ""
		}
	}

	set ST(SD,$i) $d
	set ST(SM,$i) $t

	incr ST(NS)

	# total duration hasn't changed
	recalc_profile

	set ST(MOD) 1
	set ST(RDR,PR) 1
	redraw
}

proc do_revert { } {
#
# Reverts to the original version of the profile
#
	global WN

	if { $WN(TIM) != "" } {
		# trc "revert: rundata present"
		return
	}

	if ![discard_changes] {
		return
	}

	reset_profile
	redraw
}

proc do_adjlags { } {
#
# Adjusts the segments to the lags in the current run data
#
	global WN ST

	set tt 0
	set mo 0
	for { set i 0 } { $i < $ST(NS) } { incr i } {
		set ts [expr $tt + $ST(SD,$i)]
		if { $ts < $ST(ST,$i) } {
			# we have a lag
			incr ST(SD,$i) [expr $ST(ST,$i) - $ts]
			set mo 1
		}
		incr tt $ST(SD,$i)
	}

	if $mo {
		set ST(RDR,PR) 1
		set ST(RDR,RU) 1
		redraw
	}
}

proc clear_run { } {
#
# Removes the run data from the graph
#
	global ST WN PM

	if { $ST(AST) == "R" || $WN(TIM) == "" } {
		# trc "clear_run: void"
		return
	}

	set WN(TIM) ""
	set WN(TAT) [fmtt $PM(MNT)]
	redraw_temp TAT
	array unset ST "RU,*"
	recalc_profile
	set ST(RDR,RU) 1
	set ST(RDR,PR) 1
	redraw
}

proc discard_changes { } {
#
# Checks if the current profile has been modified and warns the user
#
	global ST

	if $ST(MOD) {
		return [confirm "The current profile has been modified. Do you\
			want to discard the changes?"]
	}

	return 1
}

proc do_editsegment { } {
#
# Enter segment data by hand
#
	global WN ST Mod PROF

	if { $ST(CSE) == "" || $WN(TIM) != "" || $ST(CSE) >= $ST(NS) } {
		# trc "editsegment: void"
		return
	}

	set n $ST(CSE)
	set w [md_window "Segment $n params"]

	set ST(CSE) ""

	if { $ST(CCO) != "" } {
		wm geometry $w "+[lindex $ST(CCO) 2]+[lindex $ST(CCO) 3]"
	}

	set Mod(0,SM) $ST(SM,$n)
	set Mod(0,SD) $ST(SD,$n)

	set f [frame $w.tf]
	pack $f -side top -expand y -fill x

	label $f.ml -text "Target temperature: " -anchor w
	grid $f.ml -column 0 -row 0 -sticky w

	entry $f.me -width 10 -textvariable Mod(0,SM)
	grid $f.me -column 1 -row 0 -sticky we

	label $f.sl -text "Duration: " -anchor w
	grid $f.sl -column 0 -row 1 -sticky w

	entry $f.se -width 10 -textvariable Mod(0,SD)
	grid $f.se -column 1 -row 1 -sticky we

	#######################################################################

	set f [frame $w.bf]
	pack $f -side top -expand y -fill x

	button $f.d -text "OK" -command "md_click 1"
	pack $f.d -side right -expand n

	button $f.c -text "Cancel" -command "md_click -1"
	pack $f.c -side left -expand n

	bind $w <Destroy> "md_click -1"

	#######################################################################

	lassign $PROF(EN) ps pm

	while 1 {

		set ev [md_wait]

		if { $ev < 0 } {
			return
		}

		if { $ev == 1 } {
			# validate the changes
			if ![valid_fp_number $Mod(0,SM) \
				[lindex $pm 0] [lindex $pm 1] \
				1 tmp] {

				alert "Target temperature is invalid, it must\
					be an FP number between [lindex $pm 0]\
					and [lindex $pm 1]"
				continue
			}
			if ![valid_int_number $Mod(0,SD) \
				[lindex $ps 0] [lindex $ps 1]] {

				alert "Segment duration is invalid, it must be\
					an integer number between\
					[lindex $ps 0] and [lindex $ps 1]"
				continue
			}
			set mod 0
			if { $ST(SM,$n) != $tmp } {
				set mod 1
				set ST(SM,$n) $tmp
			}
			if { $ST(SD,$n) != $Mod(0,SD) } {
				set mod 1
				set ST(SD,$n) $Mod(0,SD)
			}
			md_stop
			if $mod {
				set ST(MOD) 1
				set tt [recalc_profile]
				if { $tt > $ST(DU) } {
					# only grow, don't shrink
					set ST(DU) $tt
					set ST(RDR,XA) 1
				}
				set ST(RDR,PR) 1
				redraw
			}
			return
		}
	}
}

proc seg_label_clear { } {
#
# Clear segment label (there can only be one at a time)
#
	global ST WN

	if { $ST(HOV) != "" } {
		set c $WN(GRP)
		foreach w $ST(HOV) {
			$c delete $w
		}
		set ST(HOV) ""
	}
}

proc ref_lines { x y } {
#
# Draw reference lines from the current point to the axes
#
	global WN

	# note: the disabled state is needed as otherwise we get an infinite
	# loop caused by the line's momentarily obscuring the point over which
	# the mouse is hovering

	set c $WN(GRP)

	set lx [$c create line $x $y [lindex $WN(GRP,S) 0] $y -dash { 3 3 } \
		-fill $WN(DLC) -state disabled]

	set ly [$c create line $x $y $x [lindex $WN(GRP,S) 1] -dash { 3 3 } \
		-fill $WN(DLC) -state disabled]

	return [list $lx $ly]
}

proc seg_label { n } {
#
# Produce a data label for node n at mouse location x y
#
	global ST WN FO

	seg_label_clear

	set c $WN(GRP)

	if { ![xstart] || ![ystart] } {
		return
	}

	# get node coordinates
	set tim 0
	for { set i 0 } { $i <= $n } { incr i } {
		# normal end time
		set ts [expr $tim + $ST(SD,$i)]
		# updated end time
		set uts $ST(ST,$i)
		if { $uts > $ts && $i != $n } {
			set tim $uts
		} else {
			# the last one, count the lag separately
			set tim $ts
		}
	}

	set tmp $ST(SM,$n)

	set x [xco [stox $tim]]
	set y [yco [ttoy $tmp]]

	set rl [ref_lines $x $y]

	if { $x < [xco [expr $WN(WID) / 2]] } {
		# we are in the left half, so the label goes to the right
		set ha "w"
		incr x $WN(LOF)
	} else {
		set ha "e"
		incr x -$WN(LOF)
	}

	if { $y > [yco [expr $WN(HEI) / 2]] } {
		# in the bottom half
		set va "s"
		incr y -$WN(LOF)
	} else {
		set va "n"
		incr y $WN(LOF)
	}

	set txt "Tmp: [format %5.1f $ST(SM,$n)]\n"
	append txt "Dur: [format %5d $ST(SD,$n)]\n"
	append txt "Tim: [format %5d $tim]"

	if { $uts != "" && $uts > $tim } {
		append txt "\nLag: [format %5d [expr $uts - $tim]]"
	}

	set tx [$c create text $x $y -anchor $va$ha -justify left -text $txt \
		-font $FO(FIX) -state disabled]

	set bg [$c create rectangle [$c bbox $tx] -fill lightgray -outline "" \
		-state disabled]

	$c raise $tx

	set ST(HOV) [concat $rl [list $tx $bg]]
}

proc move_enter { n } {
#
# Mouse hover over point, show the segment data
#
	seg_label $n
}

proc show_lines { x y } {
#
# Show lines from the current point to the axes
#
	global ST

	seg_label_clear
	set ST(HOV) [ref_lines $x $y]
}

proc move_leave { } {
#
	seg_label_clear
}

proc move_pp { n wi xx yy } {
#
# Drag profile points
#
	global ST WN

	if { $WN(TIM) != "" || $n >= $ST(NS) } {
		# you can only do this if there is no run data
		return
	}

	set c $WN(GRP)

	# determine the time range for the point
	set min 0
	for { set i 0 } { $i < $n } { incr i } {
		incr min $ST(SD,$i)
	}
	set cur [expr $min + $ST(SD,$n)]

	set last [expr $ST(NS) - 1]

	if { $n != $last } {
		# not the last segment
		set m [expr $n + 1]
		set max [expr $cur + $ST(SD,$m)]
	} else {
		set max $ST(DU)
	}

	# check if the point can be moved to this location
	set x [xctos $xx]
	set y [yctot $yy]

	if { $x < $min } {
		set x $min
	} elseif { $x > $max } {
		set x $max
	}

	set nx [xco [stox $x]]
	set ny [yco [ttoy $y]]

	if { $x != $cur || $y != $ST(SM,$n) } {
		# move and update
		set delta [expr $x - $cur]
		set ST(SD,$n) [expr $ST(SD,$n) + $delta]
		if { $n != $last } {
			set ST(SD,$m) [expr $ST(SD,$m) - $delta]
		}
		set ST(SM,$n) $y
		$c coords $wi [expr $nx - $WN(PPR)] \
			      [expr $ny - $WN(PPR)] \
			      [expr $nx + $WN(PPR)] \
			      [expr $ny + $WN(PPR)]

		set ST(RDR,PR) 1
		set ST(MOD) 1
	}

	seg_label $n
}

proc move_pp_end { n } {

	seg_label_clear
	recalc_profile
	redraw
}

proc save_profile { } {

	global ST PM WN PROF Mod

	set nl ""

	# build the list of profile names
	set cu -1
	set ix 0
	foreach p $PM(PRL) {
		set nm [lindex $p 0]
		lappend nl $nm
		if { $nm == $WN(PNA) } {
			set cu $ix
		}
		incr ix
	}

	set w [md_window "Save current profile"]

	set f [frame $w.t]
	pack $f -side top -expand y -fill x

	label $f.l -text "Name to save under: " -anchor w
	pack $f.l -side top -expand n -anchor w

	set Mod(0,SE) [ttk::combobox $f.op -values $nl]
	pack $f.op -side top -expand y -fill x

	if { $cu >= 0 } {
		$Mod(0,SE) current $cu
	}

	set f [frame $w.bf]
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
			# check if the name is legal
			set er [valid_profile_name $n]
			if { $er != "" } {
				alert "Error: $er"
				continue
			}

			# check if present already
			set ix [lsearch -exact $nl $n]
			if { $ix >= 0 } {
				# it is OK to overwrite current without warning
				if { $ix != $cu &&
					![confirm "Profile $n already exists.\
					     Do you want to overwrite?"] } {
					continue
				}
			}

			# create the profile
			set pr [build_profile $n]

			if { $ix >= 0 } {
				set PM(PRL) [lreplace $PM(PRL) $ix $ix $pr]
			} else {
				set ix [llength $PM(PRL)]
				lappend PM(PRL) $pr
			}

			set PROF(CUR) $ix

			# make sure the displayed name is up to date
			set WN(PNA) $n
			set ST(MOD) 0

			md_stop

			setsparams

			return
		}
	}
}

proc load_profile { } {
#
# Load or delete a profile from the list
#
	global ST PM WN PROF Mod

	set nl ""

	# build the list of profile names
	foreach p $PM(PRL) {
		set nm [lindex $p 0]
		lappend nl $nm
	}

	set w [md_window "Load/delete profile"]
	set f $w
	set Mod(0,SE) $WN(PNA)

	eval "tk_optionMenu $f.op Mod(0,SE) $nl"
	pack $f.op -side top -expand y -fill x

	set f [frame $w.bf]
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
		if { $v == "" } {
			continue
		}

		if { $ev == 1 } {
			if ![discard_changes] {
				continue
			}
			set ix [lsearch -exact $nl $v]
			if { $ix < 0 } {
				# impossible
				continue
			}
			set PROF(CUR) $ix
			reset_profile
			redraw
			md_stop
			return
		}

		if { $ev == 2 } {
			# delete
			set ix [lsearch -exact $nl $v]
			if { $ix < 0 } {
				# impossible
				continue
			}
			if { $v == "DEFAULT" } {
				if ![confirm "You cannot delete the DEFAULT\
					profile. Do you want to revert it to\
					the built-in DEFAULT?"] {

					continue
				}
				set PM(PRL) [lreplace $PM(PRL) $ix $ix]
				add_def_profile
				if { $WN(PNA) == "DEFAULT" } {
					reset_profile
				}
			} else {
				if ![confirm "Are you sure you want to delete\
					profile $v?"] {

					continue
				}
				set PM(PRL) [lreplace $PM(PRL) $ix $ix]
			}
			# check if the current one is still present (note that
			# its index may have changed)
			set ix [lsearch -exact -index 0 $PM(PRL) $WN(PNA)]
			if { $ix >= 0 } {
				set PROF(CUR) $ix
			} else {
				# use default
				set PROF(CUR) 0
				reset_profile
			}
			redraw
			setsparams
			md_stop
			return
		}
	}
}

proc update_widgets { } {

	global ST WN

	switch $ST(AST) {

		"R" {
			$WN(WOV) configure -state disabled
			$WN(WLB) configure -state disabled
			$WN(WSB) configure -state disabled
			$WN(WCS) configure -state disabled
			$WN(WCI) configure -state disabled
			$WN(WCD) configure -state disabled
			$WN(CEM) configure -state disabled
			$WN(WRB) configure -text "Abort" -state normal
		}

		"I" {
			$WN(WOV) configure -state normal
			$WN(WLB) configure -state normal
			$WN(WSB) configure -state normal
			$WN(WCS) configure -state normal
			$WN(WCI) configure -state normal
			$WN(WCD) configure -state normal
			$WN(CEM) configure -state normal
			$WN(WRB) configure -text "Run" -state normal
		}

		default {

			$WN(WOV) configure -state disabled
			$WN(CEM) configure -state disabled
			$WN(WLB) configure -state normal
			$WN(WSB) configure -state normal
			$WN(WCS) configure -state normal
			$WN(WCI) configure -state normal
			$WN(WCD) configure -state normal
			$WN(WRB) configure -text "Run" -state disabled
		}
	}
}

proc do_setparam { p v } {
#
# Called whenever a controller slide is moved
#
	global ST WN

	set x "PA$p"

	if { $WN($x) != $WN($x,M) } {
		# change
		if { $WN($x,M) != "" } {
			# not the first one, mark the profile as modified
			set ST(MOD) 1
		}
		set WN($x,M) $WN($x)
	}
}

proc do_updoven { v } {
#
# Called whenever the oven slide is ativated manually
#
	global ST

	if ![isstate "I"] {
		set ST(OSL) 0
		return
	}

	# trc "updoven $v"
	# lock the updates
	set ST(OSL) -1
}

proc do_setoven { } {
#
# Called when the button on the scale goes up
#
	global WN ST

	# trc "setoven $WN(OVS) $ST(OSL)"

	if { $ST(OSL) < 0 } {
		# do the update
		send_oven $WN(OVS)
		# ignore the next 3 updates
		set ST(OSL) 3
	}
}

proc do_emulation { } {
#
# Called when the emulation checkbox is clicked
#
	global WN 

	send_emulation $WN(EMU)
}

proc initialize { } {

	global ST argv PROF

	unames_init $ST(DEV) $ST(SYS)

	if { [lindex $argv 0] == "-D" } {
		set ST(DBG) 1
	}

	getsparams

	# index of the current profile
	set PROF(CUR) 0

	mk_rootwin .

	reset_profile

	update_state "D"
}

###############################################################################
###############################################################################

tip_init
	
initialize

autocn_start uart_open boss_close send_handshake handshake_ok uart_connected

while 1 { event_loop }
