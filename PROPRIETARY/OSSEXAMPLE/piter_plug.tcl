{
###############################################################################

# commands (system)
set OSS_CODE(RSA)	0xFFF
set OSS_CODE(RSB)	0xFFE
set OSS_CODE(MEMDUMP)	0xF7E
set OSS_CODE(MEMSET)	0xF7F

# commands (praxis)
set OSS_CODE(TIME)	0x000
set OSS_CODE(ECHO)	0x001
set OSS_CODE(DELAY)	0x002
set OSS_CODE(RESET)	0x003
set OSS_CODE(DIAG)	0x004

# status values
set OSS_STAT(OK)	0x0
set OSS_STAT(UNIMPL)	0x1
set OSS_STAT(ERR)	0x2
set OSS_STAT(LATER)	0x3

###############################################################################

proc pl_aw { w } {
#
# Word -> list of bytes
#
	return [format "0x%02x 0x%02x" [expr $w & 0xff] [expr ($w >> 8) & 0xff]]
}

proc pl_al { lw } {
#
# Long -> list of bytes
#
	return "[pl_aw [expr $lw & 0xffff]] [pl_aw [expr ($lw >> 16) & 0xffff]]"
}

proc pl_acmd { cm { co OK } } {
#
# Issue command bytes
#
	global OSS_CODE OSS_STAT

	pl_aw [expr ($OSS_CODE($cm) << 4) | $OSS_STAT($co)]
}

proc pl_rb { ll } {
#
# List of bytes -> simple 1-byte value
#
	upvar $ll bb

	if { [llength $bb] < 1 } {
		error "value missing"
	}
	set b [lindex $bb 0]
	if [catch { expr 0x$bb } val] {
		error "illegal byte code $b"
	}
	set bb [lrange $bb 1 end]
	return $val
}

proc pl_rw { ll } {
#
# List of bytes -> word
#
	upvar $ll bb

	if { [llength $bb] < 2 } {
		error "too few bytes"
	}
	set w "0x[lindex $bb 1][lindex $bb 0]"
	if [catch { expr $w } val] {
		error "Illegal bytes: $w"
	}
	set bb [lrange $bb 2 end]
	return $val
}

proc pl_rl { ll } {
#
# List of bytes -> lword
#
	upvar $ll bb

	if { [llength $bb] < 4 } {
		error "too few bytes"
	}

	set w "0x[lindex $bb 3][lindex $bb 2][lindex $bb 1][lindex $bb 0]"

	if [catch { expr $w } val] {
		error "Illegal bytes: $w"
	}
	set bb [lrange $bb 4 end]
	return $val
}

proc pl_rcmd { ll { st "" } } {
#
# Extract command code
#
	upvar $ll bb

	set w [pl_rw bb]

	if { $st != "" } {
		upvar $st s
		set s [pl_ename [expr $w & 0xF]]
	}

	return [pl_cname [expr $w >> 4]]
}

proc pl_cname { c } {
#
# Command code -> command name
#
	global OSS_CODE

	foreach nm [array names OSS_CODE] {
		if { $OSS_CODE($nm) == $c } {
			return $nm
		}
	}

	return $c
}

proc pl_ename { c } {
#
# Status code -> status name
#
	global OSS_STAT

	foreach nm [array names OSS_STAT] {
		if { $OSS_STAT($nm) == $c } {
			return $nm
		}
	}

	return $c
}

###############################################################################

proc plug_reset { } {
#
# Upon reset, issue the two resync messages
#
	pt_outln [pl_acmd RSA]
	pt_outln [pl_acmd RSB]
}

proc plug_init { } {

	plug_reset
}

proc plug_outpp_b { inp } {
#
# Input: list of hex values (bytes received from the UART)
#
	upvar $inp line

	set len [llength $line]

	if { $len < 2 } {
		# this is impossible
		pt_touf "PLUG_OUTPP Error: illegal sequence <$line>"
		return 0
	}

	set code [pl_rcmd line stat]
	incr len -2

	if { $code == "RSA" } {
		pt_tout "Resync 1 (ignored)"
		return 0
	}

	if { $code == "RSB" } {
		pt_touf "Resynced!"
		return 0
	}

	if { $stat != "OK" } {
		pt_tout "Command $code, $stat!"
		return 0
	}

	if { $code == "TIME" } {
		# time
		if { $len < 4 } {
			pt_touf "Illegal response length $len for TIME request"
			return 0
		}

		pt_tout "Time: [pl_rl line]"
		return 0
	}

	if { $code == "ECHO" } {
		pt_tout "Echo: <$line>"
		return 0
	}

	if { $code == "DELAY" } {
		# delay
		pt_tout "Brrrrrrnnnnnngggggg!!!"
		return 0
	}

	if { $code == "RESET" } {
		# reset, we shouldn't be receiving anything, perhaps a resync
		return 0
	}

	if { $code == "DIAG" } {
		# diag
		pt_tout "Diag OK"
		return 0
	}

	if { $code == "MEMDUMP" } {
		pt_tout "MEMORY = <$line>"
		return 0
	}

	if { $code == "MEMSET" } {
		pt_tout "MEMSET OK"
		return 0
	}
		
	pt_touf "Illegal command code in response: $code!"
	return 0
}

proc plug_inppp_b { inp } {

	upvar $inp line

	set line [string trim $line]

	if ![regexp "^(\[a-z\]+)\[ \t\]*(.*)" $line jnk cmd args] {
		pt_touf "Illegal command syntax, keyword expected"
		return 0
	}

	if { $cmd == "time" } {
		set line [pl_acmd TIME]
		return 1
	}

	if { $cmd == "echo" } {
		set line "[pl_acmd ECHO] $args"
		return 1
	}

	if { $cmd == "delay" } {
		if [catch { expr $args } val] {
			pt_touf "Illegal number: $args"
			return 0
		}
		set line "[pl_acmd DELAY] [pl_aw $val]"
		return 1
	}

	if { $cmd == "reset" } {
		set line [pl_acmd RESET]
		return 1
	}

	if { $cmd == "diag" } {
		# add the sentinel at the end of string
		set line "[pl_acmd DIAG] $args 0"
		return 1
	}

	if { $cmd == "dump" } {
		if ![regexp -nocase "^(\[^ \t\]+)\[ \t\]*(\[^ \t\]*)" $args \
		    jnk addr size] {
			pt_touf "Illegal params: address \[nbytes\] expected"
			return 0
		}
		if { [catch { expr $addr } addr] || [expr $addr & 0xffff0000]
		    != 0 } {
			pt_touf "Illegal address, must be < 0xffff"
			return 0
		}
		if { $size == "" } {
			set size 2
		} else {
			global PM
			if { [catch { expr $size } size] || $size <= 0 ||
			      $size > [expr $PM(UPL) - 2] } {
				pt_touf "Illegal size, must be > 0 and <=\
					[expr $PM(UPL) - 2]"
				return 0
			}
		}
		set line "[pl_acmd MEMDUMP] [pl_aw $addr] [pl_aw $size]"
		return 1
	}

	if { $cmd == "memset" } {
		if ![regexp -nocase "^(\[^ \t\]+)\[ \t\]*(.+)" $args \
		    jnk addr bytes] {
			pt_touf "Illegal params: address bytelist expected"
			return 0
		}
		if { [catch { expr $addr } addr] || [expr $addr & 0xffff0000]
		    != 0 } {
			pt_touf "Illegal address, must be < 0xffff"
			return 0
		}
		set line "[pl_acmd MEMSET] [pl_aw $addr] $bytes"
		return 1
	}

	if { $cmd == "resync" } {
		plug_reset
		return 0
	}

	pt_touf "Illegal command: $cmd"
	return 0
}

###############################################################################
}
