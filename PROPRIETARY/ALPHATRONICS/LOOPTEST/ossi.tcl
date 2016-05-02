set ACKCODE(0)		"OK"
set ACKCODE(1)		"Command format error"
set ACKCODE(2)		"Illegal length of command packet"
set ACKCODE(3)		"Illegal command parameter"
set ACKCODE(4)		"Illegal command code"
set ACKCODE(6)		"Module is off"
set ACKCODE(7)		"Module is busy"
set ACKCODE(8)		"Temporarily out of resources"

set ACKCODE(129) 	"Command format error, rejected by AP"
set ACKCODE(130)	"AP command format error"
set ACKCODE(131)	"Command too long for RF, rejected by AP"

proc myinit { } {

}

myinit

#############################################################################
#############################################################################

oss_interface -id 0x00010022 -speed 115200 -length 56 \
	-parser { parse_cmd show_msg }

#############################################################################
#############################################################################
##
## Commands:
##
##	ping
##	turn [on | off]
##
##	radio [on] [delay]	(sets the delay, cannot off->on)
##	radio off
##
##	ap -node ... -wake ... -retries
##

oss_command ping 0x01 {
#
	byte	pval;
}

oss_command turn 0x02 {
#
# Turn the device on/off
#
	byte	what;
}

oss_command dump 0x03 {
#
# Memory dump
#
	word	addr;
	word	size;
}

oss_command radio 0x06 {
#
# Set radio delay
#
        # 0 == off
        word    delay;
}

oss_command ap 0x80 {
#
# Access point configuration
#
	# WOR wake retry count
	byte	worp;
	# Regular packet retry count
	byte	norp;
	# WOR preamble length
	word	worprl;
	# Node ID
	word	nodeid;
}

#############################################################################
#############################################################################

oss_message status 0x01 {
#
# Status info; sensor value
#
	lword	tstamp;
	lword	status;
}

oss_message dump 0x03 {
#
# Memory dump
#
	blob	bytes;
}

oss_message ap 0x80 {
#
# To be extended later
#
	byte	worp;
	byte	norp;
	word	worprl;
	word	nodeid;
}

##############################################################################
##############################################################################

proc parse_selector { } {

	return [oss_parse -skip -match {^-([[:alnum:]]+)} -return 2]
}

proc parse_value { sel min max } {

	set val [oss_parse -skip -number -return 1]

	if { $val == "" } {
		error "$sel, illegal value"
	}

	if [catch { oss_valint $val $min $max } val] {
		error "$sel, illegal value, $val"
	}

	return $val
}

proc parse_check_empty { } {

	set cc [oss_parse -skip " \t," -match ".*" -return 1]
	if { $cc != "" } {
		error "superfluous arguments: $cc"
	}
}

variable CMDS

set CMDS(ping)		"parse_cmd_ping"
set CMDS(dump)		"parse_cmd_dump"
set CMDS(turn)		"parse_cmd_turn"
set CMDS(radio)		"parse_cmd_radio"
set CMDS(ap)		"parse_cmd_ap"

set LASTCMD		""

proc parse_cmd { line } {

	variable CMDS
	variable LASTCMD

	set cc [oss_parse -start $line -skip -return 0]
	if { $cc == "" || $cc == "#" } {
		# empty or comment
		return
	}

	if { $cc == "!" } {
		if { $LASTCMD == "" } {
			error "no previous command"
		}
		parse_cmd $LASTCMD
		return
	}

	if { $cc == ":" } {
		# a script
		set cc [string trim \
			[oss_parse -match "." -match ".*" -return 1]]
		set res [oss_evalscript $cc]
		oss_ttyout $res
		return
	}

	set cmd [oss_parse -subst -skip -match {^[[:alpha:]_][[:alnum:]_]*} \
		-return 2]

	if { $cmd == "" } {
		# no keyword
		error "illegal command syntax, must start with a keyword"
	}

	set cc [oss_keymatch $cmd [array names CMDS]]
	oss_parse -skip

	# check for a (generally) optional alpha keyword following the command
	# word

	$CMDS($cc) [oss_parse -match {^[[:alpha:]]+} -skip -return 0]

	set LASTCMD $line
}

##############################################################################

proc parse_cmd_ping { what } {
#
#
#
	set val [oss_parse -skip -number -return 1]
	if { $val == "" } {
		set val 0
	}

	parse_check_empty

	oss_issuecommand 0x01 [oss_setvalues [list $val] "ping"]
}

proc parse_cmd_turn { what } {

	if { $what != "" } {
		set kl { "on" "off" }
		if [catch { oss_keymatch $what $kl } what] {
			error "expected one of on, off"
		}
	}

	if { $what == "" || $what == "on" } {
		set what 1
	} else {
		set what 0
	}

	parse_check_empty

	oss_issuecommand 0x02 [oss_setvalues [list $what] "turn"]
}

proc parse_cmd_dump { what } {

	if { $what != "" } {
		error "unexpected $what"
	}

	set val [oss_parse -skip -number -return 1]

	if { $val == "" } {
		set val 65535
		set len 0
	} else {
		if { $val < 0 || $val > 65534 } {
			error "illegal address"
		}
		set len [oss_parse -skip -number -return 1]
		if { $len == "" } {
			set len 1
		} elseif { $len < 1 || $len > 32 } {
			error "illegal length, must be 1 to 32"
		}
	}

	parse_check_empty

	oss_issuecommand 0x03 [oss_setvalues [list $val $len] "dump"]
}

proc parse_cmd_radio { what } {

	if { $what != "" } {
		set kl { "on" "off" }
		if [catch { oss_keymatch $what $kl } what] {
			error "expected one of on, off"
		}
	}

	if { $what == "" || $what == "on" } {
		# optional delay
		set val [oss_parse -skip -number -return 1]
		if { $val == "" } {
			if { $what == "" } {
				error "expected one of on, off"
			}
			# the default
			set val 2048
		}
	} else {
		# off
		set val 0
	}

	parse_check_empty

	oss_issuecommand 0x06 [oss_setvalues [list $val] "radio"]
}

proc parse_cmd_ap { what } {

	if { $what != "" } {
		error "unexpected $what"
	}

	# unused
	set nodeid 0xFFFF
	set worprl 0xFFFF
	set worp 0xFF
	set norp 0xFF

	while 1 {

		set tp [parse_selector]
		if { $tp == "" } {
			break
		}

		set k [oss_keymatch $tp { "node" "wake" "retries" "preamble" }]

		if { $k == "node" } {
			if { $nodeid != 0xFFFF } {
				error "duplicate node specfification"
			}
			set nodeid [parse_value "node" 1 65534]
			continue
		}

		if { $k == "wake" } {
			if { $worp != 0xFF } {
				error "duplicate wor specification"
			}
			set worp [parse_value "wor" 0 3]
			continue
		}

		if { $k == "preamble" } {
			if { $worprl != 0xFFFF } {
				error "duplicate preamble specification"
			}
			set worprl [parse_value "preamble" 1023 10240]
			continue
		}

		if { $norp != 0xFF } {
			error "duplicate retries specification"
		}

		set norp [parse_value "retries" 0 7]
	}

	parse_check_empty
	oss_issuecommand 0x80 \
		[oss_setvalues [list $worp $norp $worprl $nodeid ] "ap"]
}

###############################################################################

proc show_msg { code ref msg } {

	if { $code == 0 } {
		# ACK or NAK
		if { $ref != 0 } {
			variable ACKCODE
			binary scan $msg su msg
			if [info exists ACKCODE($msg)] {
				oss_ttyout "<$ref>: $ACKCODE($msg)"
			} else {
				oss_ttyout "<$ref>: response code $msg"
			}
		}
		return
	}

	set str [oss_getmsgstruct $code name]

	if { $str == "" } {
		# this will trigger default dump
		error "no user support"
	}

	show_msg_$name $msg
}

proc show_msg_status { msg } {

	lassign [oss_getvalues $msg "status"] tst sta

	set res "Time $tst --> [format %08X $sta]"

	oss_ttyout $res
}

proc show_msg_dump { msg } {

	lassign [oss_getvalues $msg "dump"] mem

	set res "DMP:"

	foreach m $mem {
		append res " [format %02X $m]"
	}

	oss_ttyout $res
}

proc show_msg_ap { msg } {

	lassign [oss_getvalues $msg "ap"] worp norp worprl nodeid

	set res "AP status:\n"
	append res "  WOR packets: $worp\n"
	append res "  Retries:     $norp\n"
	append res "  Preamble:    $worprl\n"
	append res "  Node:        $nodeid ([format %04X $nodeid])\n"

	oss_ttyout $res
}

###############################################################################
###############################################################################
