#!/usr/bin/wish
###############

			###############################
			##                           ##
			## P. Gburzynski, March 2007 ##
			##                           ##
			###############################

set Debug 		1

set ESNLIST 		""
set MaxLineCount	1024

# number of samples per seconds (one sample here is the transfer/storage unit,
# i.e., 4 ADC samples
set SAMP(SPS)		125
# max sampling time in seconds
set SAMP(MAX)		600


array set CMD		{ BIND 0 UNBIND 64 RESET 16 STOP 32 ABORT 33 REPORT 48
				SAMPLE 80 SAMPLEX 82 HRMON 112 HRMOFF 113
					SEND 96 HELLO 128 HRATE 208 SDATA 192
							STATUS 144 }

array set INTV		{ BIND 1000 BINDTRIES 10 ERRLINGER 5000 PACKET 10
				SENDREQ 500 ACKSKIP 4 ACKRETR 600 STATREP 3000
					FAILACK 16 FAILABT 50 CLEANER 30000 
						UNBINDTIME 30 }

set CHAR(PRE)		[format %c [expr 0x55]]
set CHAR(0)		\x00

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

#
# Status variables:
#
# LST = major state: IDL, SMP, DNL, RST
# LSI = 1 - init, 0 - in progress
# LSR = 1 - h/s receiving samples (SAF used), 0 - not receiving samples
# HMO = 0 - HR off, 1 - HR starting, 2 - HR started
# STA = displayed status: READY, DOWNLOADING, SAMPLING
# TSI = displayed Sample Id: state-related Id of the sample being processed
# LEF = displayed remaining to go in seconds
# REM = remaining to go
# FRM = starting sample
# UPT = upto, i.e., last sample + 1
# LPR = last prompt (to avoid recreating them for persistent prompting)
# PCB = prompt callback
#


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
		puts "$hdr:$ol"
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

	# note: the rate will become flexible later
	fconfigure $SFD -mode "115200,n,8,1" -handshake none \
		-buffering none -translation binary -blocking 0

	set SIO(TIM)  ""
	set SIO(STA) -1

	fileevent $SFD readable r_serial
}

proc put1 { v } {

	global SIO

	append SIO(MSG) [binary format c $v]
}

proc put2 { v } {

	global SIO

	append SIO(MSG) [binary format s $v]
}

proc put3 { v } {

	global SIO

	append SIO(MSG) [binary format cs $v [expr $v >> 8]]
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

proc get3 { } {

	global SIO

	set w 0
	set b 0
	binary scan $SIO(BUF) cs b w

	set SIO(BUF) [string range $SIO(BUF) 3 end]

	return [expr (($w & 0xffff) << 8) | ($b & 0xff)]
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

	set SIO(MSG) [binary format S $lid]
}

proc handle_hello { } {
#
# FIXME: may want to check if the node has no window, i.e., it has become
# unbound by accident.
#
	global Wins SIO

	set esn [get4]

	if { $esn == 0 } {
		log "received 0 ESN in HELLO, ignored"
		return
	}

	addESN $esn
}

proc val_count { v } {

	if { $v == "" } {
		return 1
	}

	return [expr [regexp "^\[1-9\]\[0-9\]*$" $v] && ($v < 0x00ffffff)]
}

proc val_sid { v } {

	if { $v == "" } {
		return 1
	}

	return [expr [regexp "^\[1-9\]\[0-9\]*$" $v] && ($v < 0x00ff)]
}

proc busy_buttons { lid { all 0 } } {
#
# Make sure some buttons are disabled while busy
#
	global Wins

	set wn $Wins($lid)

	$wn.bot.act.unb configure -state disabled
	$wn.bot.act.res configure -state disabled
	$wn.bot.smp.but configure -state disabled
	$wn.bot.smp.buu configure -state disabled
	$wn.bot.ret.but configure -state disabled
	$wn.bot.ret.sid configure -state disabled

	if $all {
		$wn.bot.act.sto configure -state disabled
	} else {
		$wn.bot.act.sto configure -state active
	}
}

proc idle_buttons { lid } {
#
# Make sure some buttons are disabled while busy
#
	global Wins

	set wn $Wins($lid)

	$wn.bot.act.unb configure -state active
	$wn.bot.act.res configure -state active
	$wn.bot.smp.but configure -state active
	$wn.bot.smp.buu configure -state active
	$wn.bot.ret.but configure -state active
	$wn.bot.ret.sid configure -state active

	# this one too
	$wn.bot.act.sto configure -state active
}

proc sample_found { lid sid } {

	global Wins

	# check if the sample is already there
	foreach sl $Wins($lid,SLI) {
		if { $sid == [lindex $sl 0] } {
			return 1
		}
	}

	return 0
}

proc start_window { esn lid } {

	global Wins FixedFont

	set wn ".n$lid"
	set Wins($lid) $wn
	toplevel $wn

	wm title $wn "ESN [dspe $esn] on LID $lid"
	wm resizable $wn 0 0

	labelframe $wn.top -text Status -padx 2 -pady 2
	pack $wn.top -side top -expand 0 -fill x

	label $wn.top.stat -textvariable Wins($lid,STA)
	pack $wn.top.stat -side left -expand 0 -fill none

	label $wn.top.left -width 8 -textvariable Wins($lid,LEF) -font \
		$FixedFont -relief sunken
	pack $wn.top.left -side right -expand 0 -fill none

	label $wn.top.leftl -text "    Left:"
	pack $wn.top.leftl -side right -expand 0 -fill none

	label $wn.top.csi -width 3 -textvariable Wins($lid,TSI) -font \
		$FixedFont -relief sunken
	pack $wn.top.csi -side right -expand 0 -fill none

	label $wn.top.csil -text "   CSID:"
	pack $wn.top.csil -side right -expand 0 -fill x

	#######################################################################

	frame $wn.bot -borderwidth 2
	pack $wn.bot -side bottom -expand 0 -fill x

	#######################################################################

	labelframe $wn.bot.ret -text "Retrieval" -padx 2 -pady 2
	grid $wn.bot.ret -column 0 -row 1 -sticky wns -padx 8 -pady 8

	###

	menubutton $wn.bot.ret.sid -font $FixedFont -text "      --      " \
		-underline -1 -direction below -menu $wn.bot.ret.sid.m \
			-relief raised
	menu $wn.bot.ret.sid.m -tearoff 0
	grid $wn.bot.ret.sid -column 1 -row 0 -sticky we -pady 4 -padx 4

	label $wn.bot.ret.sil -text "Sample: "
	grid $wn.bot.ret.sil -column 0 -row 0 -sticky e

	###

	entry $wn.bot.ret.frm -relief sunken \
		-textvariable Wins($lid,EFR) -validate key \
			-vcmd "val_count %P" -invcmd bell \
				-font $FixedFont
	grid $wn.bot.ret.frm -column 1 -row 1 -sticky we -pady 4 -padx 4

	label $wn.bot.ret.frl -text "From: "
	grid $wn.bot.ret.frl -column 0 -row 1 -sticky e

	###

	entry $wn.bot.ret.cnt -relief sunken \
		-textvariable Wins($lid,ECT) -validate key \
			-vcmd "val_count %P" -invcmd bell \
				-font $FixedFont
	grid $wn.bot.ret.cnt -column 1 -row 2 -sticky we -pady 4 -padx 4

	label $wn.bot.ret.cnl -text "Count: "
	grid $wn.bot.ret.cnl -column 0 -row 2 -sticky e

	button $wn.bot.ret.but -text Download -command "start_download $lid"
	grid $wn.bot.ret.but -column 1 -row 3 -sticky we -pady 4 -padx 4

	#######################################################################

	labelframe $wn.bot.smp -text Sampling -padx 2 -pady 2
	grid $wn.bot.smp -column 1 -row 1 -sticky wnse -padx 8 -pady 8

	###

	entry $wn.bot.smp.sid -relief sunken \
		-textvariable Wins($lid,SID) -validate key \
			-vcmd "val_sid %P" -invcmd bell \
				-font $FixedFont
	grid $wn.bot.smp.sid -column 1 -row 0 -sticky we -pady 4 -padx 4

	label $wn.bot.smp.sil -text "Sample ID: "
	grid $wn.bot.smp.sil -column 0 -row 0 -sticky e

	###

	entry $wn.bot.smp.cnt -relief sunken -width 8 \
		-textvariable Wins($lid,SCN) -validate key \
			-vcmd "val_count %P" -invcmd bell \
				-font $FixedFont
	grid $wn.bot.smp.cnt -column 1 -row 1 -sticky we -pady 4 -padx 4

	label $wn.bot.smp.cnl -text "Count: "
	grid $wn.bot.smp.cnl -column 0 -row 1 -sticky e

	###

	button $wn.bot.smp.but -text "Start Sampling" \
		-command "start_sampling $lid 0"
	grid $wn.bot.smp.but -column 1 -row 2 -sticky we -pady 4 -padx 4

	###

	button $wn.bot.smp.buu -text "Sample & Send " \
		-command "start_sampling $lid 1"
	grid $wn.bot.smp.buu -column 1 -row 3 -sticky we -pady 4 -padx 4

	#######################################################################

	labelframe $wn.bot.hmo -text "Heart Rate Monitor" -padx 2 -pady 2
	grid $wn.bot.hmo -column 1 -row 0 -sticky we -padx 8 -pady 8

	###

	label $wn.bot.hmo.rat -width 3 -textvariable Wins($lid,HRA) -font \
		$FixedFont -relief sunken

	pack $wn.bot.hmo.rat -side left -expand 0 -fill none

	label $wn.bot.hmo.ral -text "b/s   "
	pack $wn.bot.hmo.ral -side left -expand 0 -fill x

	button $wn.bot.hmo.raf -text Stop -command "stop_hmo $lid"
	pack $wn.bot.hmo.raf -side right -expand 0 -fill none

	button $wn.bot.hmo.ras -text Start -command "start_hmo $lid"
	pack $wn.bot.hmo.ras -side right -expand 0 -fill none

	#######################################################################

	labelframe $wn.bot.act -text "Control" -padx 2 -pady 2
	grid $wn.bot.act -column 0 -row 0 -sticky we -padx 8 -pady 8

	button $wn.bot.act.sto -text Stop -command "full_stop $lid"
	pack $wn.bot.act.sto -side left -expand 0 -fill none

	button $wn.bot.act.unb -text Unbind -command "unbind $lid"
	pack $wn.bot.act.unb -side left -expand 0 -fill none

	button $wn.bot.act.res -text Reset -command "reset $lid"
	pack $wn.bot.act.res -side right -expand 0 -fill none

	#######################################################################

	bind $wn <Destroy> "disconnect $lid"
}

proc redo_sample_menu { lid } {

	global Wins SAMP

	set wn $Wins($lid)

	# the present top label
	set lab [$wn.bot.ret.sid cget -text]
	$wn.bot.ret.sid.m delete 0 end

	set nf -1
	set ix 0
	foreach sl $Wins($lid,SLI) {
		set tla [format "%03d <%5.1f>" [lindex $sl 0] \
			[expr double ([lindex $sl 1]) / $SAMP(SPS)]]
		if { $lab == $tla } {
			set nf $ix
		}
		$wn.bot.ret.sid.m add command -label $tla \
			-command "select_sample_menu $lid $ix"
		incr ix
	}

	if { $nf < 0 } {
		$wn.bot.ret.sid.m activate none
		$wn.bot.ret.sid configure -text "      --      "
	} else {
		$wn.bot.ret.sid.m activate $nf
	}
}

proc select_sample_menu { lid ix } {

	global Wins SAMP

	set wn $Wins($lid)

	set sl [lindex $Wins($lid,SLI) $ix]

	if { $sl == "" } {
		log "illegal sample menu index $ix"
		return
	}

	$wn.bot.ret.sid.m activate $ix
	$wn.bot.ret.sid configure \
		-text [format "%03d <%5.1f>" [lindex $sl 0] \
			[expr double ([lindex $sl 1]) / $SAMP(SPS)]]
}

proc unbind { lid } {

	global Wins

	destroy $Wins($lid)
}

proc disconnect { lid } {

	global Wins SAF

	if [info exists Wins($lid)] {
		after 20
		send_cmd $lid UNBIND
		after 20
		log "LID $lid, disconnecting, unbinding"
		if { $Wins($lid,SCB) != "" } {
			catch { after cancel $Wins($lid,SCB) }
		}
		if { $Wins($lid,PCB) != "" } {
			catch { after cancel $Wins($lid,PCB) }
		}
		# deallocate the structures
		unset Wins($lid)
		array unset Wins $lid,*
		array unset SAF $lid,*
	}
}

proc send_reset_prompt { lid } {

	global Wins INTV

	if { $Wins($lid,LST) != "RST" } {
		# not needed any more
		return
	}

	send_cmd $lid RESET

	set Wins($lid,SCB) [after $INTV(ACKRETR) "send_reset_prompt $lid"]
}

proc reset { lid } {

	global Wins

	full_stop $lid

	set Wins($lid,LST) "RST"
	set Wins($lid,STA) "RESETTING"

	log "LID $lid, resetting ..."

	send_reset_prompt $lid
}

proc runit { } {

	global Wins SIO CMD

### DUMP
    	# dump "RCV" $SIO(BUF)
### ####

	# extract the command first; if not for us, there is no need to
	# verify CRC
	binary scan $SIO(BUF) Sc SIO(LID) cmd

	if [expr ($cmd & 0x80) == 0] {
		# AP packet, ignore
		return
	}

	# validate CRC
	if [w_chk $SIO(BUF)] {
		log "illegal checksum, packet ignored"
		return
	}

	# strip off the checksum as well
	set SIO(BUF) [string range $SIO(BUF) 3 end-2]

	if { [expr ($cmd & 0xf0)] == $CMD(STATUS) }  {
		set st [expr ($cmd >> 1) & 0x3]
		set mo [expr $cmd & 0x1]
		handle_status $st $mo
		return
	}

	set cmd [expr $cmd & 0xff]

	if { $cmd == $CMD(HELLO) } {
		handle_hello
	} else {
		# we must have a window for any other message
		if ![info exists Wins($SIO(LID))] {
			log "LID $SIO(LID), bound packet from disconnected node"
			send_cmd $SIO(LID) UNBIND
			return
		}
		if { $cmd == $CMD(SDATA) } {
			handle_sdata $SIO(LID)
		} elseif { $cmd == $CMD(HRATE) } {
			handle_hrate $SIO(LID)
		} else {
			log "LID $SIO(LID), illegal packet type $cmd"
		}
	}
}

proc w_serial { } {

	global SFD SIO CHAR

	set ln [expr [string length $SIO(MSG)] - 4]

	puts -nonewline $SFD $CHAR(PRE)
	puts -nonewline $SFD [binary format c $ln]
	puts -nonewline $SFD $SIO(MSG)

### DUMP
    dump "SND <$ln>" $SIO(MSG)
### ####
	unset SIO(MSG)
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
					log "packet timeout $SIO(STA)"
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
			log "illegal packet length $bl"
			set SIO(STA) -1
			continue
		}

		set SIO(STA) [expr $bl + 4]
		set SIO(BUF) ""
	}
}
			
proc addText { w txt } {

	$w configure -state normal
	$w insert end "$txt"
	$w configure -state disabled
	$w yview -pickplace end

}

proc endLine { w } {

	global	MaxLineCount
	
	$w configure -state normal
	$w insert end "\n"

	set ix [$w index end]
	set ix [string range $ix 0 [expr [string first "." $ix] - 1]]

	if { $ix > $MaxLineCount } {
		# scroll out the topmost line if above limit
		$w delete 1.0 2.0
	}

	$w configure -state disabled
	# make sure the last line is displayed
	$w yview -pickplace end

}

proc dspe  { u } {
#
# Display node ESN
#
	return [format %010u $u]
}


proc log { txt } {
#
# Writes a line to log
#
	global Logger

	while 1 {

		set el [string first "\n" $txt]
		if { $el < 0 } {
			set el [string length $txt]
		}
		incr el -1
		set out [string range $txt 0 $el]
		incr el +2
		set txt [string range $txt $el end]
		addText $Logger \
			"[clock format [clock seconds] -format %H:%M:%S] $out"
		endLine $Logger
		if { $txt == "" } {
			return
		}
	}
}

proc bind_clear { esn } {

	global WBind

	if ![info exists WBind($esn)] {
		return
	}

	if { $WBind($esn,TIM) != "" } {
		catch { after cancel $WBind($esn,TIM) }
	}

	array unset WBind $esn
	array unset WBind "$esn,*"
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

proc binder { esn } {

	global WBind SIO CMD INTV

	if ![info exists WBind($esn)] {
		# just in case
		return
	}

	# mark "no timer"
	set WBind($esn,TIM) ""

	if { $WBind($esn) >= $INTV(BINDTRIES) } {
		log "bind for [dspe $esn] failed: timeout"
		bind_clear $esn
		return
	}

	if [info exists WBind($esn,MSG)] {
		set SIO(MSG) $WBind($esn,MSG)
	} else {
		# create the bind message
		init_msg $WBind($esn,LID)
		put1 $CMD(BIND)
		put4 $esn
		msg_close
		set WBind($esn,MSG) $SIO(MSG)
	}
	# send the bind message
	w_serial

	incr WBind($esn)

	# keep trying
	set WBind($esn,TIM) [after $INTV(BIND) "binder $esn"]
}

proc send_cmd { lid cmd } {

	global SIO CMD

	init_msg $lid
	put1 $CMD($cmd)
	msg_close

	w_serial
}

proc lid_alloc { } {

	global Wins LastLid

	while 1 {
		set lid $LastLid
		incr LastLid
		# Zero and this are reserved
		if [expr $LastLid == 0xffff] {
			set LastLid 1
		}

		if ![info exists Wins($lid)] {
			return $lid
		}
	}
}

proc isbound { esn } {
#
# Return LID or zero if no window
#
	global Wins

	foreach es [array names Wins "*,LID"] {
		if { $Wins($es) == $esn } {
			regexp "^\[^,\]+" $es es
			return $es
		}
	}

	return 0
}

proc bindESN { u } {

	global WBind ESNLIST

	# get the ESN
	set u [lindex [lindex $ESNLIST $u] 0]

	if [isbound $u] {
		# already bound
		log "ESN [dspe $u] already bound"
		return
	}

	if ![info exists WBind($u)] {
		# startup a binder
		set WBind($u) 0
		set WBind($u,LID) [lid_alloc]
		set WBind($u,TIM) ""
		log "waiting for [dspe $u] to respond on lid $WBind($u,LID) ..."
		binder $u
	}
}

proc addESN { new } {

	global ESNLIST

	set ol ""
	set ct [clock seconds]

	set found 0
	foreach el $ESNLIST {

		set esn [lindex $el 0]
		set tim [lindex $el 1]

		if { $esn == $new } {
			set found 1
			lappend ol [list $esn $ct]
		} else {
			lappend ol $el
		}
	}

	if $found {
		set ESNLIST $ol
		return
	}

	lappend ESNLIST [list $new $ct]

	set ESNLIST [lsort -index 0 $ESNLIST]

	# re-display the list
	updateESN
}

proc cleanESN { } {

	global ESNLIST INTV

	set ol ""
	set ct [clock seconds]

	set deleted 0
	foreach el $ESNLIST {

		if { [expr $ct - [lindex $el 1]] > $INTV(UNBINDTIME) } {
			incr deleted
		} else {
			append ol $el
		}
	}

	if $deleted {
		set ESNLIST $ol
		updateESN
	}
}

proc cleaner { } {

	global INTV

	cleanESN

	after $INTV(CLEANER) cleaner
}

proc updateESN { } {

	global ESNLIST ESNL

	# delete everything
	$ESNL configure -state normal
	$ESNL delete 1.0 end

	set ne 0
	foreach el $ESNLIST {
		$ESNL insert end "[format %010u [lindex $el 0]]\n" "H$ne"
		incr ne
	}

	# hyperlink bindings
	for { set i 0 } { $i < $ne } { incr i } {
		set t "H$i"
		$ESNL tag bind $t <Any-Enter> "$ESNL tag configure $t \
			-background #10F010 -relief raised -borderwidth 1"
		$ESNL tag bind $t <Any-Leave> "$ESNL tag configure $t \
			-background {} -relief flat"
		$ESNL tag bind $t <1> "bindESN $i"
	}
	$ESNL configure -state disabled
}

proc update_left { lid } {

	global Wins SAMP

	set Wins($lid,LEF) [format "%1.1f" [expr double ($Wins($lid,REM)) / \
		$SAMP(SPS)]]
}

proc handle_status { st mo } {
#
# Process STATUS messages
#
	global Wins SIO INTV SAMP

	set lid $SIO(LID)

	if ![info exists Wins($lid)] {
		# no window, check if waiting to be bound
		global WBind

		set bad 1
		foreach es [array names WBind "*,LID"] {
			if { $WBind($es) == $lid } {
				regexp "^\[^,\]+" $es es
				bind_clear $es
				log "LID $lid, [dspe $es] bound"
				start_window $es $lid
				# initial state
				set Wins($lid,LST) "IDL"
				set Wins($lid,STA) "READY"
				set Wins($lid,LSI) 0
				set Wins($lid,LSR) 0
				# sample list
				set Wins($lid,SLI) ""
				# prompt callback
				set Wins($lid,PCB) ""
				# status callback
				set Wins($lid,SCB) ""
				# HRM state
				set Wins($lid,HMO) 0
				set bad 0
				idle_buttons $lid
				kick_status_updater $lid
			}
		}

		if $bad {
			log "spurious status over LID $lid"
			send_cmd $lid UNBIND
			return
		}
	}

	if { $st == 0 } {
		# ready
		set Wins($lid,LEF) ""
		set Wins($lid,TSI) ""
		set sl ""
		while { [string length $SIO(BUF)] > 3 } {
			set sid [get1]
			set sle [get3]
			lappend sl [list $sid $sle]
		}

		if { $Wins($lid,SLI) != $sl } {
			# sample list has been updated
			set Wins($lid,SLI) $sl
			# redo the menu
			redo_sample_menu $lid
		}

		if { $Wins($lid,LST) == "RST" && $sl == "" } {
			# done resetting
			set Wins($lid,LST) "IDL"
			set Wins($lid,STA) "READY"
			if { $Wins($lid,PCB) != "" } {
				catch { after cancel $Wins($lid,PCB) }
				set Wins($lid,PCB) ""
			}
			log "LID $lid, reset complete"
			return
		}

		if { $Wins($lid,LST) == "SMP" } {
			# we were sampling
			if { !$Wins($lid,LSI) || \
			    		[sample_found $lid $Wins($lid,TSI)] } {
				# either we were not initializing (i.e., fully
				# in progress, or we were initializing and lost
				# the status race (the sample has been collected
				# while we were not looking); both cases mean
				# that we are done
				if $Wins($lid,LSR) {
					# we were simultaneously receiving
					# samples
					if { $Wins($lid,REM) == 0 } {
						# we were lucky; nothing left
						log "LID $lid, all received"
						finish_reception $lid
						# finish_reception changes LST
						return
					}
					# no such luck, enter download mode
					set Wins($lid,LST) "DNL"
					set Wins($lid,STA) "DOWNLOADING"
					# initializing
					set Wins($lid,LSI) 1
					# LSR already set
					make_transmission_prompt $lid
					send_transmission_prompt $lid
					return
				}
				# sampling done
				set Wins($lid,LST) "IDL"
				set Wins($lid,STA) "READY"
				set Wins($lid,LSR) 0
				idle_buttons $lid
			}
			return
		}
	
		if { $Wins($lid,LST) == "DNL" } {
			if !$Wins($lid,LSI) {
				# this cannot happen
				log "LID $lid, reception interrupted by node"
				full_stop $lid
			}
		}
		return
	}

	if { $st == 1 } {
		# sampling
		if { $Wins($lid,LST) != "SMP" && $Wins($lid,LSR) == 0 } {
			# we haven't ordered that
			log "LID $lid, unrequested sampling"
			# in case we have been receiving as well: this will
			# also reset our status
			full_stop $lid
			return
		}

		set sid [get1]
		if { $sid != $Wins($lid,TSI) } {
			log "LID $lid, sampling the wrong sample"
			full_stop $lid
			return
		}

		if $Wins($lid,LSI) {
			# was initializing
			set Wins($lid,LSI) 0
			if { $Wins($lid,LPR) != "" } {
				catch { after cancel $Wins($lid,LPR) }
				set Wins($lid,LPR) ""
			}
		}

		if !$Wins($lid,LSR) {
			# not receiving; this is the only way to update the
			# remaining count; otherwise (i.e., receiving), we
			# will be doing this on received samples
			set snu [get3]
			set Wins($lid,REM) [expr $Wins($lid,UPT) - \
				$Wins($lid,FRM) - $snu]
			# display "remaining"
			update_left $lid
		}

		return
	}

	if { $st == 2 } {
		# sending
		if { $Wins($lid,LST) != "DNL" } {
			# somebody is confused
			send_cmd $lid ABORT
			log "spurious sending status on LID $lid"
		}

		if $Wins($lid,LSI) {
			# was initializing
			set Wins($lid,LSI) 0
			if { $Wins($lid,LPR) != "" } {
				catch { after cancel $Wins($lid,LPR) }
				set Wins($lid,LPR) ""
			}
		}

		return
	}

	log "illegal status $st on LID $lid"
}

proc start_sampling { lid rcv } {

	global Wins SAF SAMP SIO CMD

	if { $Wins($lid,LST) != "IDL" } {
		# this cannot happen
		log "LID $lid, node busy, sampling request ignored"
		return
	}

	# get the assigned sample number
	if { [catch { expr $Wins($lid,SID) } sid] || $sid < 0 || $sid > 255 } {
		log "LID $lid, empty or illegal sample number"
		return
	}

	# check if the sample is already there
	if [sample_found $lid $sid] {
		log "LID $lid, sample $sid already present"
		return
	}

	# get the length in seconds
	if { [catch { expr $Wins($lid,SCN) } sle] || $sle < 1 ||
	    $sle > $SAMP(MAX) } {
		log "LID $lid, illegal or null sampling count: $sle"
		return
	}

	busy_buttons $lid
	set Wins($lid,LST) "SMP"
	set Wins($lid,STA) "SAMPLING"
	set Wins($lid,LSI) 1
	# current sample
	set Wins($lid,TSI) $sid

	set sle [expr $sle * $SAMP(SPS)]
	set Wins($lid,FRM) 0
	set Wins($lid,UPT) $sle
	set Wins($lid,REM) $sle
	update_left $lid

	# create the prompt message
	init_msg $lid

	if $rcv {
		put1 $CMD(SAMPLEX)
		# will be receiving them on-line
		set Wins($lid,LSR) 1
		for { set i 0 } { $i < $sle } { incr i } {
			set SAF($lid,$i) ""
		}
	} else {
		put1 $CMD(SAMPLE)
	}

	put1 $sid
	put3 $sle
	msg_close

	set Wins($lid,LPR) $SIO(MSG)
	send_sampling_prompt $lid
}

proc send_sampling_prompt { lid } {

	global Wins SIO INTV

	if { $Wins($lid,LST) != "SMP" || $Wins($lid,LSI) == 0 } {
		# we are not needed any more
		set Wins($lid,PCB) ""
		set Wins($lid,LPR) ""
		return
	}

	set SIO(MSG) $Wins($lid,LPR)
	w_serial
	set Wins($lid,PCB) [after $INTV(SENDREQ) "send_sampling_prompt $lid"]
}

proc start_download { lid } {

	global Wins SAF INTV SAMP

	set wn $Wins($lid)

	if { $Wins($lid,LST) != "IDL" } {
		log "LID $lid, node busy, download request ignored"
		return
	}

	# get the parameters
	if { $Wins($lid,SLI) == "" } {
		log "LID $lid, node has no samples to download"
		return
	}

	# check for selection
	set ix [$wn.bot.ret.sid.m index active]
	if [catch { expr $ix } ix] {
		log "LID $lid, no sample selected for download"
		return
	}

	set sl [lindex $Wins($lid,SLI) $ix]
	set sid [lindex $sl 0]
	set sle [lindex $sl 1]

	# check the origin
	set frm $Wins($lid,EFR)
	if { $frm == "" } {
		set frm 0
	} elseif { [catch { expr $frm * $SAMP(SPS) } frm] || $frm >= $sle } {
		# convert seconds to samples
		log "LID $lid, sample offset out of range, request ignored"
		return
	}

	set cnt $Wins($lid,ECT)
	if { $cnt == "" } {
		# upto
		set cnt $sle
	} elseif { [catch { expr $frm + ($cnt * $SAMP(SPS)) } cnt] } {

		log "LID $lid, sample count illegal, request ignored"
		return

	}

	if { $cnt > $sle } {
		set cnt $sle
	}

	# so far, so good
	busy_buttons $lid
	log "LID $lid, requesting sample $sid <$frm, $cnt> ..."

	set Wins($lid,LST) "DNL"
	set Wins($lid,STA) "DOWNLOADING"
	set Wins($lid,LSI) 1
	set Wins($lid,LSR) 1
	set Wins($lid,TSI) $sid

	# initialize the sample index
	set Wins($lid,FRM) $frm
	# the limit
	set Wins($lid,UPT) $cnt

	for { set i $frm } { $i < $cnt } { incr i } {
		set SAF($lid,$i) ""
	}

	# remaining
	set Wins($lid,REM) [expr $cnt - $frm]
	update_left $lid

	make_transmission_prompt $lid
	send_transmission_prompt $lid
}

proc make_transmission_prompt { lid } {

	global Wins SAF SIO CMD

	init_msg $lid
	put1 $CMD(SEND)
	put1 $Wins($lid,TSI)

	set n 0

	set i $Wins($lid,FRM)
	set l $Wins($lid,UPT)

	while { $i != $l } {
		# opening a new range
		if { $SAF($lid,$i) != "" } {
			incr i
			continue
		}

		if { $n == 15 } {
			# only one left
			put3 $i
			break
		}

		set s $i
		incr i

		while { $i != $l } {
			if { $SAF($lid,$i) != "" } {
				break
			}
			incr i
		}

		set t [expr $i - 1]
		if { $s == $t } {
			# a singleton
			put3 $s
			incr n
			continue
		}

		# a range
		put3 $t
		put3 $s

		incr n 2

		if { $n == 16} {
			break
		}
	}

	if { $n == 0 } {
		log "LID $lid, illagal call to mtp, no missing samples"
		return
	}

	msg_close

	set Wins($lid,LPR) $SIO(MSG)
}

proc send_transmission_prompt { lid } {

	global Wins SIO INTV

	if { $Wins($lid,LST) != "DNL" || $Wins($lid,LSI) == 0 } {
		# stop sending the prompt
		set Wins($lid,PCB) ""
		set Wins($lid,LPR) ""
		return
	}

	set SIO(MSG) $Wins($lid,LPR)
	w_serial
	set Wins($lid,PCB) [after $INTV(ACKRETR) \
						"send_transmission_prompt $lid"]
}

proc handle_sdata { lid } {

	global Wins SIO SAF

	# sample ID
	set sid [get1]

	if { $Wins($lid,LSR) == 0 } {
		# not expecting anything like this, just ignore
		return
	}

	if { $sid != $Wins($lid,TSI) } {
		# something fundamentally wrong
		log "LID $lid, unexpected sample ID, operation aborted
		full_stop $lid
		return
	}

	set hra [get1]
	set len [string length $SIO(BUF)]

	if { $Wins($lid,HMO) && $hra != $Wins($lid,HRA) } {
		# this may look stupid, but setting this variable triggers a
		# window update, so we don't want to do it unnecessarily
		set Wins($lid,HRA) $hra
	}

	if $Wins($lid,LSI) {
		# initializing
		if { $len < 51 } {
			# ignore EOR if initializing; it belongs to the previous
			# round
			return
		}
		# mark as initalized
		set Wins($lid,LSI) 0
		if { $Wins($lid,LPR) != "" } {
			catch { after cancel $Wins($lid,LPR) }
			set Wins($lid,LPR) ""
		}
	}

	if { $len < 51 } {
		# EOR
		if { $Wins($lid,REM) == 0 } {
			log "LID $lid, all done"
			# all done
			finish_reception $lid
			return
		}
		log "LID $lid, EOR, $Wins($lid,REM) left"
		# next round
		set Wins($lid,LST) "DNL"
		set Wins($lid,STA) "DOWNLOADING"
		set Wins($lid,LSI) 1
		# LSR already set
		make_transmission_prompt $lid
		send_transmission_prompt $lid
		return
	}

	set frm [get3]

	if ![info exists SAF($lid,$frm)] {
		log "LID $lid, illegal sampling number $frm, reception aborted"
		full_stop $lid
		return
	}

	if { $SAF($lid,$frm) == "" } {
		set SAF($lid,$frm) $SIO(BUF)
		incr Wins($lid,REM) -1
		if { [expr $Wins($lid,REM) % 125] == 0 } {
			update_left $lid
		}
	}
}

proc kick_status_updater { lid } {

	global Wins

	if $Wins($lid,LSR) {
		# not needed if receiving
		return
	}

	if { $Wins($lid,SCB) != "" } {
		catch { after cancel $Wins($lid,SCB) }
		set Wins($lid,SCB) ""
	}

	status_updater $lid
}

proc status_updater { lid } {

	global Wins INTV

	if $Wins($lid,LSR) {
		# don't need this at all; as a matter of fact, the updater
		# is in the way
		set Wins($lid,SCB) ""
		return
	}

	if { $Wins($lid,HMO) == 1 } {
		# starting heart monitor
		send_cmd $lid HRMON
		# fast
		set intv $INTV(SENDREQ)
	} elseif { $Wins($lid,LST) == "SMP" } {
		# sampling, no transfer
		send_cmd $lid REPORT
		set intv $INTV(SENDREQ)
	} else {
		if { $Wins($lid,LST) == "IDL" } {
			if $Wins($lid,HMO) {
				send_cmd $lid STOP
			} else {
				send_cmd $lid ABORT
			}
		} else {
			send_cmd $lid REPORT
		}
		set intv $INTV(STATREP)
	}

	set Wins($lid,SCB) [after $intv "status_updater $lid"]
}

proc full_stop { lid } {

	global Wins

	if { $Wins($lid,PCB) != "" } {
		# prompt callback
		catch { after cancel $Wins($lid,PCB) }
		set Wins($lid,PCB) ""
	}

	if $Wins($lid,LSR) {
		# stop reception
		array unset SAF "$lid,*"
		set Wins($lid,LSR) 0
	}

	set Wins($lid,LST) "IDL"
	set Wins($lid,STA) "READY"
	set Wins($lid,LSI) 0
	set Wins($lid,LPR) ""

	kick_status_updater $lid

	idle_buttons $lid
}

proc finish_reception { lid } {

	global Wins

	# unset this, so full_stop won't erase the samples
	set Wins($lid,LSR) 0
	full_stop $lid
	# block everything
	busy_buttons $lid 1
	
	# save dialog
	set wn ".f$lid"
	toplevel $wn

	wm title $wn "LID $lid: saving sample $Wins($lid,TSI)"
	wm resizable $wn 1 0

	frame $wn.top
	pack $wn.top -side top

	label $wn.top.m -text "Where to save sample $Wins($lid,TSI): "
	pack $wn.top.m -side left
	entry $wn.top.ent -width 20
	pack $wn.top.ent -side left -expand 1 -fill x

	button $wn.top.bro -text "Browse ..." \
		-command "save_file_dialog $wn $wn.top.ent"
	pack $wn.top.bro -side left -expand 0 -fill none

	frame $wn.bot
	pack $wn.bot -side top

	button $wn.bot.sav -text Save -command "do_save $lid $wn"
	pack $wn.bot.sav -side right

	button $wn.bot.dis -text Discard -command "do_discard $lid $wn"
	pack $wn.bot.dis -side left

	# we shall wait for the dialog to go away
}

proc save_file_dialog { wn en } {

    	set types {
		{ "Sample files"	{ .smp .samp }	}
		{ "All files"		* 		}
    	}

	set file [tk_getSaveFile -filetypes $types -parent $wn \
	    -initialfile Untitled -defaultextension .smp ]

	if { $file != "" } {
		$en delete 0 end
		$en insert 0 $file
		$en xview end
	}
}

proc do_save { lid wn } {

	global Wins SAF

	set fn [$wn.top.ent get]

	if { $fn == "" } {
		log "LID $lid, save ignored, no file specified"
		return
	}

	# try to open the file for writing

	if [catch { open $fn "w" } wfd] {
		log "LID $lid, unable to write to file '$fn'"
		return
	}

	log "LID $lid, saving ..."

	fconfigure $wfd -translation binary

	for { set sm $Wins($lid,FRM) } { $sm < $Wins($lid,UPT) } { incr sm } {
		if [catch { puts -nonewline $wfd $SAF($lid,$sm) } err] {
			log "LID $lid, write error: $err, save aborted"
			return
		}
	}

	log "LID $lid, sample saved"

	array unset SAF "$lid,*"

	# destroy the dialog

	destroy $wn

	idle_buttons $lid
}

proc do_discard { lid wn } {

	global Wins SAF

	log "LID $lid, sample discarded"

	array unset SAF "$lid,*"
	destroy $wn
	idle_buttons $lid
}

proc handle_hrate { lid } {

	global Wins

	if { $Wins($lid,HMO) == 0 } {
		# we don't wont this
		send_cmd $lid HRMOFF
		return
	}

	if { $Wins($lid,HMO) == 1 } {
		set Wins($lid,HMO) 2
	}

	#
	set Wins($lid,HRA) [get1]
}

proc start_hmo { lid } {

	global Wins

	if $Wins($lid,HMO) {
		log "LID $lid, heart monitor already running"
		return
	}

	set Wins($lid,HMO) 1
	kick_status_updater $lid
}

proc stop_hmo { lid } {

	global Wins

	if { $Wins($lid,HMO) == 0 } {
		log "LID $lid, heart monitor isn't running"
		return
	}

	set Wins($lid,HMO) 0
	set Wins($lid,HRA) ""
	send_cmd $lid HRMOFF
}

###############################################################################

set FixedFont	{-family courier -size 9}

wm title . "HM AP listening"

######### the ESN list window #################################################

set esf [frame .left -borderwidth 2]
pack $esf -side left -expand no -fill y

set ESNL [text $esf.esns -yscrollcommand "$esf.scroll set" -setgrid true \
	-borderwidth 2 -relief sunken \
	-width 11 -height 24 -font $FixedFont]

pack $ESNL -side left -expand no -fill y

scrollbar $esf.scroll -command "$ESNL yview"
pack $esf.scroll -side right -fill y

unset esf

######### the Logger ##########################################################

set lof [frame .right -borderwidth 2]
pack $lof -side right -expand yes -fill both

set Logger [text $lof.t -width 64 -height 24 \
	-borderwidth 2 -relief raised -setgrid true -wrap char \
	-yscrollcommand "$lof.scrolly set" \
	-font $FixedFont \
	-exportselection 1 \
	-state normal]

scrollbar $lof.scrolly -command "$lof.t yview"

pack $lof.scrolly -side right -fill y

pack $Logger -side right -expand yes -fill both

unset lof

######### COM port ############################################################

set cpn [lindex $argv 0]
puts $cpn

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
	after $INTV(ERRLINGER) "exit"
	set cpn ""
	unset SFD
}

unset cpn

###############################################################################

set LastLid	1

if [info exists SFD] {
	log "Listening ..."
	listen
}

after $INTV(CLEANER) cleaner
