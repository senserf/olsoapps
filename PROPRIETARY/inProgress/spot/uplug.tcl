{

###############################################################################
###############################################################################

proc plug_input { in tag } {

	global PLUG

	upvar $in inp

	set inp [string trim $inp]

	if [regexp -nocase {^m[[:alpha:]]*} $inp mat] {
		# master [n]
		set inp [string trimleft \
			[string range $inp [string length $mat] end]]
		if [catch { expr $inp } no] {
			set no $PLUG($tag,NODEID)
			if { $no == "" } {
				pt_tout "Node Id still unknown,\
					command ignored!" $tag
				return 1
			}
		}

		if { $no > 0xFFFE || $no < 0 } {
			pt_tout "Illegal master node id $no, command ignored!" $tag
			return 1
		}

		set PLUG($tag,MASTER) $no
		pt_tout "Master node Id = [format %04X $no] ($no)" $tag
		return 1
	}

	if [regexp -nocase {^d[[:alpha:]]*} $inp mat] {
		# dump
		set inp [string trimleft \
			[string range $inp [string length $mat] end]]
		if [catch { expr $inp } df] {
			pt_tout "Number expected!" $tag
			return 1
		}
		set PLUG($tag,DUMP) $df
		return 1
	}

	# direct

	set lv ""

	if [regexp -nocase {^x} $inp] {
		# line in hex
		set inp [string range $inp 1 end]

		while 1 {

			set inp [string trimleft $inp]
			if { $inp == "" } {
				break
			}
			set dd [string range $inp 0 1]
			set inp [string range $inp 2 end]

			if [catch { expr 0x$dd } val] {
				pt_tout "Bad input: $dd is not a pair of hex\
					digits, ignored!" $tag
				return 1
			}
			lappend lv $val
		}
	} else {
		# free style numbers/expressions
		while 1 {
			set inp [string trimleft $inp]
			if { $inp == "" } {
				break
			}
		
			if [catch { plug_parse_number inp } val] {
				pt_tout "Bad input: $inp ($val), ignored!" $tag
				return 1
			}

			if [catch { plug_validate_int $val 0 255 } n] {
				pt_tout "Illegal byte value: $val,\
					input ignored!" $tag
				return 1
			}
			lappend lv $n
		}
	}

	if { [llength $lv ] < 2 } {
		pt_tout "Need at least two bytes!" $tag
		return 1
	}

	set lf [expr [llength $lv] + 3]
	set ls [lindex $lv 1]

	if { $ls != $lf } {
		pt_tout "Fixing length $ls to $lf" $tag
	}

	set lv [lreplace $lv 1 1 $lf]

	plug_dmp $lv "-->" $tag
	pt_outln $lv $tag

	return 1
}

proc show_message { inp tag } {

	global PLUG

	set org $inp
	set ln [llength $org]

	if { $ln < 5 } {
		# garbage
		plug_dmp $org "<-U" $tag
		return
	}

	lassign $org tp tl nl nh no
	set inp [lrange $org 4 end]

	if { $tp == 0x91 && $ln >= 8 } {
		# response to "get node info"
		if { $PLUG($tag,PENDING) == $tp } {
			# awaited
			set PLUG($tag,PENDING) ""
			plug_dmp $org "<-W" $tag 2
		} else {
			# spurious
			plug_dmp $org "<-S" $tag 1
		}
		set ii [expr [lindex $inp 0] | ([lindex $inp 1] << 8)]
		if { $ii == 1 } {
			set ni [expr [lindex $inp 2] | \
				([lindex $inp 3] << 8)]
			if { $PLUG($tag,NODEID) == "" } {
				pt_tout "Node Id = [format %04X $ni] ($ni)" $tag
				set PLUG($tag,NODEID) $ni
			} elseif { $PLUG($tag,NODEID) != $ni } {
				pt_tout "Node Id change:\
					[format %04X $PLUG($tag,NODEID)]\
					($PLUG($tag,NODEID)) -->\
					[format %04X $ni] ($ni)" $tag
				set PLUG($tag,NODEID) $ni
			}
		}
		# more?
		return 0
	}

	if { $tp == 0x92 } {
		# ACK for "set node info"
		if { $PLUG($tag,PENDING) == $tp } {
			# awaited
			set PLUG($tag,PENDING) ""
			plug_dmp $org "<-W" $tag 2
		} else {
			# spurious
			plug_dmp $org "<-S" $tag 1
		}
		return 0
	}

	if { $tp == 0x01 && $ln >= 16 } {
		plug_dmp $org "<-E" $tag
		set pi [expr [lindex $inp  0] | ([lindex $inp 1] << 8)]
		set ti [expr [lindex $inp  2] | ([lindex $inp 3] << 8)]
		set ts [expr [lindex $inp  6]]
		if { $pi == $PLUG($tag,NODEID) } {
			set PLUG($tag,EV,$ti) $ts
		}
		set bu [expr [lindex $inp  4]]
		set it [expr [lindex $inp  5]]
		set vo [expr [lindex $inp  7]]
		set rs [expr [lindex $inp  8]]
		set tx [expr [lindex $inp  9]]
		set ad [expr [lindex $inp 10]]
		set ag [expr [lindex $inp 11]]
		pt_tout "Event, peg = [format %04X $pi] ($pi),\
			tag = [format %04X $ti] ($ti), ad = $ad, sn = $ts:" $tag
			
		# WLO: initial hack
		set retr [expr (($it >> 4) & 7)]
		if { $it & 0x80 } {
			set noack " ?"
		} else {
			set noack ""
		}
		
		if { $it & 0x0f } {
			set it "G $retr$noack"
		} else {
			set it "L $retr$noack"
		}
		pt_tout "    button:   $bu ($it)" $tag
		set vo [expr ((($vo << 3) + 1000.0) / 4095.0) * 5.0]
		pt_tout "    voltage:  [format %4.2f $vo]" $tag
		pt_tout "    rssi:     $rs" $tag
		pt_tout "    txpower:  $tx" $tag
		pt_tout "    age:      $ag" $tag
		# send ACK
		set out "0x81 0x08 $nl $nh 0x06"
		plug_dmp $out "A->" $tag
		pt_outln $out $tag
		return 0
	}

	plug_dmp $org "<-U" $tag
	return 0
}

proc plug_timeout_start { del tag } {

	global PLUG

	if { $PLUG($tag,CB,TIMEOUT) != "" } {
		catch { after cancel $PLUG($tag,CB,TIMEOUT) }
	}

	set PLUG($tag,CB,TIMEOUT) [after $del plug_timeout_gooff $tag]
}

proc plug_timeout_gooff { tag } {

	global PLUG

	set PLUG($tag,PENDING) $PLUG($tag,PENDING)
	set PLUG($tag,CB,TO) ""
}

proc plug_timeout_clear { tag } {

	global PLUG

	if { $PLUG($tag,CB,TIMEOUT) != "" } {
		catch { after cancel $PLUG($tag,CB,TIMEOUT) }
		set PLUG($tag,CB,TIMEOUT) ""
	}
}

proc plug_renesas { tag } {

	global PLUG

	while 1 {

###	#######################################################################
###	#######################################################################

	if ![info exists PLUG($tag,MASTER)] {
		return
	}

	set ml [expr {  $PLUG($tag,MASTER)       & 0xFF }]
	set mh [expr { ($PLUG($tag,MASTER) << 8) & 0xFF }]

	switch $PLUG($tag,RSTATE) {

	0 {
		# start for get node info
		set PLUG($tag,TR) 1
		set out "0x11 0x09 0x00 0x00 $ml $mh"
		plug_dmp $out "P->" $tag 2
		pt_outln $out $tag
		set PLUG($tag,PENDING) 0x91
		set PLUG($tag,RSTATE) 1
		plug_timeout_start $PLUG($tag,RTIME) $tag
		vwait PLUG($tag,PENDING)
	}

	1 {
		plug_timeout_clear $tag
		if { $PLUG($tag,PENDING) != "" && $PLUG($tag,TR) < $PLUG($tag,RETRIES) } {
			set out "0x11 0x09 0x00 0x00 $ml $mh"
			plug_dmp $out "P->" $tag 3
			pt_outln $out $tag
			incr PLUG($tag,RETRIES)
			plug_timeout_start $PLUG($tag,RTIME) $tag
		} elseif { $PLUG($tag,NODEID) == "" } {
			# still don't know the node Id, keep polling
			set PLUG($tag,RSTATE) 0
			plug_timeout_start $PLUG($tag,LTIME) $tag
		} else {
			set PLUG($tag,TR) 1
			set out "0x12 0x0B 0x00 0x00 0x02 0x00 $ml $mh"
			plug_dmp $out "P->" $tag 2
			pt_outln $out $tag
			set PLUG($tag,PENDING) 0x92
			set PLUG($tag,RSTATE) 2
			plug_timeout_start $PLUG($tag,RTIME) $tag
		}
		vwait PLUG($tag,PENDING)
	}

	2 {
		plug_timeout_clear $tag
		if { $PLUG($tag,PENDING) != "" && $PLUG($tag,TR) < $PLUG($tag,RETRIES) } {
			set out "0x12 0x0B 0x00 0x00 0x02 0x00 $ml $mh"
			plug_dmp $out "P->" $tag 3
			pt_outln $out $tag
			incr PLUG($tag,RETRIES)
			plug_timeout_start $PLUG($tag,RTIME) $tag
		} else {
			# close the loop
			set PLUG($tag,RSTATE) 0
			plug_timeout_start $PLUG($tag,PTIME) $tag
		}
		vwait PLUG($tag,PENDING)
	}
	}

###	#######################################################################
###	#######################################################################
	}
}
			
proc plug_validate_int { n { min "" } { max "" } } {
#
# Validate an integer value
#
	set n [string tolower [string trim $n]]
	if { $n == "" } {
		error "empty string"
	}

	if { [string first "." $n] >= 0 || [string first "e" $n] >= 0 } {
		error "not an integer number"
	}

	if [catch { expr $n } n] {
		error "not a number"
	}

	if { $min != "" && $n < $min } {
		error "must not be less than $min"
	}

	if { $max != "" && $n > $max } {
		error "must not be greater than $max"
	}

	return $n
}

proc plug_parse_number { l } {
#
# Parses something that should amount to a number, which can be an expression
#
	upvar $l line

	if [regexp {^[[:space:]]*"} $line] {
		# a special check for a string which fares fine as an
		# expression, but we don't want it; if it doesn't open the
		# expression, but occurs inside, that's fine
		error "illegal \" at the beginning of expression"
	}

	set ll [string length $line]
	set ix $ll

	while 1 {

		incr ix -1
		if { $ix < 0 } {
			# failure
			error "illegal number"
		}

		if ![catch { expr [string range $line 0 $ix] } res] {
			# found, remove the match
			set line [string range $line [expr $ix + 1] end]
			return $res
		}
		# check for the special case of illegal octal number
		set so [string range $line $ix [expr $ix + 1]]
		if [regexp {[0-7][8-9]} $so] {
			error "illegal octal digit in '$so'"
		}
	}
}

proc plug_toh { lv } {

	set res ""

	foreach v $lv {
		lappend res [format "%02x" $v]
	}

	return [join $res]
}

proc plug_dmp { lv t tag { lev 1 } } {

	global PLUG

	if { $PLUG($tag,DUMP) >= $lev } {
		if { $t == "" } {
			set t "-"
		}
		pt_tout "${t}: \[[plug_toh $lv]\]" $tag
	}
}

###############################################################################

proc pt_tout { m tag } {

	term_output "$m\n" $tag
}

proc pt_outln { m tag } {


	set r [list 0x02]
	set h [expr { (-(0x02 + 0x03)) & 0xFF } ]

	foreach v $m {

		if { $v == 0x02 || $v == 0x03 || $v == 0x10 } {
			lappend r 0x10
		}

		lappend r $v

		set h [expr { ($h - $v) & 0xFF } ]
	}

	if { $h == 0x02 || $h == 0x03 || $h == 0x10 } {
		lappend r 0x10
	}

	lappend r $h
	lappend r 0x03

	set out ""

	foreach v $r {
		append out [binary format c $v]
	}

	uart_send $out $tag
}

proc plug_open { nodenum hostnum nodetype tag } {
#
# List: nodenum, hostnum, total, nodetype (txt)
#
	global PLUG

	if { $nodetype != "a321p" } {
		return 0
	}

	set PLUG($nodenum,T) $tag
	set PLUG($tag,N) $nodenum
	set PLUG($tag,S) 0
	term_output "Welcome to the VUEE model of AP321, host $hostnum\n\n" $tag

	set PLUG($tag,MASTER) 1
	set PLUG($tag,DUMP) 0

	# node Id (unknown yet)
	set PLUG($tag,NODEID) ""

	# pending command
	set PLUG($tag,PENDING) ""

	# number of failures
	set PLUG($tag,FAILURES) 0

	# the renesas thread
	set PLUG($tag,RSTATE) 0
	# this is broken; you cannot vwait in a plugin
	# set PLUG($tag,CB,RENESAS) [after 1 plug_renesas $tag]
	set PLUG($tag,CB,TIMEOUT) ""

	# Retransmission interval
	set PLUG($tag,RTIME)	100
	# Longer interval for persistent polling for Node Id
	set PLUG($tag,LTIME) 500
	# Poll time
	set PLUG($tag,PTIME) 2000

	# Number of retries
	set PLUG($tag,RETRIES) 3

	return 1
}

###############################################################################

proc plug_receive { bytes tag hexflag } {

	global PLUG

	set chunk ""
	while { $bytes != "" } {
		set c [string index $bytes 0]
		set bytes [string range $bytes 1 end]
		scan $c %c v
		lappend chunk $v
	}

	while 1 {

		set bl [llength $chunk]
		if { $bl == 0 } {
			return ""
		}

		switch $PLUG($tag,S) {

		0 {
			# waiting for STX
			for { set i 0 } { $i < $bl } { incr i } {
				set c [lindex $chunk $i]
				if { $c == 2 } {
					# STX
					set PLUG($tag,S) 1
					set PLUG($tag,E) 0
					set PLUG($tag,B) ""
					set PLUG($tag,L) 0
					# initialize the parity byte
					set PLUG($tag,P) 5
					break
				}
			}
			if { $i == $bl } {
				return ""
			}
			incr i
			set chunk [lrange $chunk $i end]
		}

		1 {
			# parse for escapes
			for { set i 0 } { $i < $bl } { incr i } {
				set c [lindex $chunk $i]
				if !$PLUG($tag,E) {
					if { $c == 3 } {
						# that's it
						set PLUG($tag,S) 0
						set chunk [lrange $chunk \
							[expr { $i + 1 }] end]
						if $PLUG($tag,P) {
							pt_tout "Parity error!"\
								$tag
							break
						}
						# remove the parity byte
						show_message [lrange \
						    $PLUG($tag,B) 0 end-1] $tag
						break
					}
					if { $c == 2 } {
						# reset
						set PLUG($tag,S) 0
						set chunk [lrange $chunk $i end]
						break
					}
					if { $c == 16 } {
						# escape
						set PLUG($tag,E) 1
						continue
					}
				} else {
					set PLUG($tag,E) 0
				}
				lappend PLUG($tag,B) $c
				set PLUG($tag,P) \
					[expr { ($PLUG($tag,P) + $c) & 0xFF }]
				incr PLUG($tag,L)
			}
			if $PLUG($tag,S) {
				set chunk ""
			}
		}

		}
	}
}

proc plug_close { tag } {

	global PLUG

	# kill all callbacks
	foreach c [array names PLUG "$tag,CB,*"] {
		catch { after cancel $PLUG($c) }
	}

	if [info exists PLUG($tag,N)] {
		set n $PLUG($tag,N)
		unset PLUG($n,T)
		array unset PLUG "$tag,*"
	}
}

###############################################################################

}
