#!/bin/sh
###########################\
exec wish "$0" "$@"

# version number
set PM(VER)	1.2

# macimum number of ports to try
set PM(MPN)	32


proc log { msg } {
#
# temporary
#
	puts $msg
}

proc fclose { fd } {

	catch { close $fd }

}

proc u_tryopen { pn } {
#
# Tries to open UART port pn
#
	foreach dev "COM${pn}: /dev/ttyUSB$pn /dev/tty$pn" {
		if ![catch { open $dev "r+" } fd] {
			return $fd
		}
	}

	return ""
}

proc u_preopen { } {
#
# Determine which COM ports are openable
#
	global PM

	set res ""

	for { set pn 0 } { $pn <= $PM(MPN) } { incr pn } {
		set fd [u_tryopen $pn]
		if { $fd != "" } {
			append res " $pn"
			fclose $fd
		}
	}

	return $res
}

proc u_conf { prt speed } {
#
# Centralized fconfigure for the UART
#
	global WN

	set fd $WN(FD,$prt)

	fconfigure $fd -mode "$speed,n,8,1" -handshake none \
		-eofchar "" -translation auto -buffering line -blocking 0

	fileevent $fd readable "u_rdline $prt"
}

proc u_rdline { prt } {

	global WN

	set fd $WN(FD,$prt)

	if [catch { gets $fd line } nc] { return }

	set line [string trim $line]
	if { $line == "" } {
		return
	}

	if { $WN(EC,$prt) != "" } {
		# logging
		echo_line $prt "-> $line"
	}

	if { $WN(EX,$prt) != "" } {
		# expecting something
		if [regexp -nocase $WN(EX,$prt) $line] {
			set WN(LI,$prt) $line
		}
	}

	if { $WN(SH,$prt) && [regexp "^1002 " $line] } {
		show_sensors $prt $line
	}

	if !$WN(SE,$prt) {
		return
	}

	#######################################################################
	### sample extraction #################################################
	#######################################################################

	set em $WN(SE,$prt)

	if { $em < 3 } {
		extract_aggregator_samples $prt $em $line
	} else {
		extract_collector_samples $prt $em $line
	}
}

proc preconf_port { prt fd } {
#
# Preconfigure the port's record, given the UART descriptor
#
	global WN

	set WN($prt) ""

	set WN(FD,$prt) $fd

	# logging
	set WN(EC,$prt) ""

	# waiting for some specific line (contains the pattern)
	set WN(EX,$prt) ""

	# showing sensor values
	set WN(SH,$prt) 0

	# extracting samples
	set WN(SE,$prt) 0

	# expected line (just in case)
	set WN(LI,$prt) ""

	# node type: unknown yet
	set WN(NT,$prt) ""

	# timeout link
	set WN(TM,$prt) ""
}

proc stop_port { pn } {
#
# Terminate the indicated connection
#
	global WN

	if [info exists WN($pn)] {
		catch { destroy $WN($pn) }
		unset WN($pn)
	}

	if [info exists WN(FD,$pn)] {
		# UART open
		fclose $WN(FD,$pn)
		unset WN(FD,$pn)
	}

	# NOTE: this is incomplete

	foreach nm [array names WN "*,$pn"] {
		unset WN($nm)
	}
}

proc try_port { prt } {
#
# Check if a node is responding at the port and determine its type
#
	global WN PM

	set ret [issue $prt "h" "^\[12\]001.* commands" 3 2 nowarn]

	if { $ret < 0 } {
		# abort
		return -1
	}


###here + on destroyed message



	if [issue $prt "h" "^\[12\]001.* commands" 3 2 nowarn] {
		# failure
		return 0
	}

	set r $WN(LI,$prt)

	if { [string first "Aggregator" $r] >= 0 } {
		set nt "aggregator"
	} elseif { [string first "Collector" $r] >= 0 } {
		set nt "collector"
	} elseif { [string first "Custodian" $r] >= 0 } {
		set nt "custodian"
	} else {
		# unknown node type
		alert "Node responding on port $prt has an unknown type, it\
			will be ignored!"
		return 0
	}

	# check the version
	if ![regexp "\[123\]\\.\[0-9\]" $r ver] {
		set ver "unknown"
	}

	if { $ver != $PM(VER) } {
		alert "Node ($nt) responding on port $prt runs software version\
			$ver, which is incompatible with required $PM(VER). The\
			node will be ignored!"
		return 0
	}

	set WN(NT,$prt) $nt
	return 1
}

proc itmout { prt } {

	global WN

	set WN(LI,$prt) "@T"
}

###here
proc cancel_issue { prt } {
#
# Cancells an 'issue' in progress
#
	global WN

	if { $WN(TM,$prt) != "" } {
		catch { after cancel $WN(TM,$prt) }
		set WN(TM,$prt) ""
		set WN(LI,$prt) "@A"
	}
}

proc issue { prt cmd pat ret del { nowarn "" } } {
#
# Issues a command to the node
#
	global WN

	set WN(EX,$prt) $pat
	# make sure it is set
	set WN(LI,$prt) "@@"

	while { $ret > 0 } {
		if { $WN(EC,$prt) != "" } {
			echo_line $prt "<- $cmd"
		}
		catch { puts $WN(FD,$prt) $cmd }
		set tm [after [expr $del * 1000] "itmout $prt"]
		vwait WN(LI,$prt)
		catch { after cancel $tm }
		set rv $WN(LI,$prt)
		if { $rv == "@A" } {
			# abort
			set WN(EX,$prt) ""
			return -1
		}
		if { $rv != "@T" } {
			# the line: success
			set WN(EX,$prt) ""
			return 0
		}
		# keep waiting
		incr ret -1
	}
	# failure
	set WN(EX,$prt) ""
	if { $nowarn == "" } {
		alert "The $WN(NT,$prt) on port $prt failed to respond on time!"
	}
	return 1
}

###############################################################################

proc alert { msg } {

	tk_dialog .alert "Attention!" $msg "" 0 "OK"
}

proc disable_main_window { } {

	.conn.connect configure -state disabled

}

proc enable_main_window { } {

	.conn.connect configure -state normal

}

proc mk_mess_window { parent w h txt } {


	set wn "${parent}.msg"

	if ![winfo exists $wn] {

		# if already exists, just change the message (not sure what to
		# do, but this should never happen)

		toplevel $wn
		label $wn.t -width $w -height $h -borderwidth 2 -state normal
		pack $wn.t -side top -fill x -fill y
		button $wn.c -text "Cancel" -command { cancel_connect }
		pack $wn.c -side top
		bind $wn <Destroy> { cancel_connect }
	}

	$wn.t configure -text $txt

	return $wn
}

proc setup_window { prt } {

	global WN

puts "setup_window $prt -> $WN(NT,$prt)"

}

proc do_connect { } {

	global WN PM ST mpVal mrVal

	if [catch { expr $mpVal } prt] {
		set prt ""
	}

	if [catch { expr $mrVal } spd] {
		set spd ""
	}

	if { $prt != "" } {
		if [info exists WN(FD,$prt)] {
			alert "Port $prt already used in another session,\
				close that session first!"
			return
		}
	}

	set nty ""

	disable_main_window

	# create a message window to show the progress

	set msgw [mk_mess_window "" 35 1 ""]
	set fail 1

	if { $spd == "" } {
		set splist "19200 9600"
	} else {
		set splist $spd
	}

	if { $prt == "" } {
		# scan ports
		for { set prt 0 } { $prt <= $PM(MPN) } { incr prt } {

			if [info exists WN(FD,$prt)] {
				# already open, skip this one
				continue
			}

			set fd [u_tryopen $prt]

			if { $fd == "" } {
				# not available
				continue
			}

			# tentatively preconfigure things without openining the
			# window yet

			preconf_port $prt $fd

			foreach sp $splist {

				u_conf $prt $sp

				if [catch { 
					$msgw.t configure -text \
						"Connecting to port $prt at $sp"
				}] {
					# cancelled
cancelled!
				}

				if [try_port $prt] {
					# success
					set fail 0
					break
				}
			}

			if !$fail {
				break
			}

			# failure: close the port and try again
			stop_port $prt
		}

		if $fail {
			alert "Failed to find a responding node!"
		}

	} else  {

		# explicit port

		set fd [u_tryopen $prt]

		if { $fd != "" } {

			preconf_port $prt $fd

			foreach sp $splist {

				u_conf $prt $sp

				if [catch {
					$msgw.t configure -text \
						"Connecting to port $prt at $sp"
				}[ {
cancelled!
				}

				if [try_port $prt] {
					# success
					set fail 0
					break
				}
			}

			if $fail {
				alert "Node doesn't respond!"
				stop_port $prt
			}

		} else {
			alert "The port could not be opened!"
		}
	}

	destroy $msgw

	if !$fail {
		setup_window $prt
	} 

	enable_main_window
}

# Startup #####################################################################

wm title . "EcoNnect $PM(VER)"

# use a frame: we may want to add something to the window later
frame .conn -borderwidth 10
pack .conn -side top -expand 0 -fill x

label .conn.lr -text "Rate:"
tk_optionMenu .conn.mr mrVal "Auto" "9600" "19200"

set cpl [u_preopen]

label .conn.lp -text "Port:"

eval "tk_optionMenu .conn.mp mpVal Auto[u_preopen]"

button .conn.connect -text "Connect" -command do_connect

button .conn.quit -text "Quit" -command { destroy .conn }

pack .conn.lr .conn.mr .conn.lp .conn.mp -side left
pack .conn.quit .conn.connect -side right

bind .conn <Destroy> { exit }
bind . <Destroy> { exit}
