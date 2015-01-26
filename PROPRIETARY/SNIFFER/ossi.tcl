oss_interface -id 0x00010007 -speed 115200 -length 82 \
	-parser { parse_cmd show_msg }

###############################################################################
###############################################################################

oss_command status 0x01 {
#
# This is a dummy at present
#
	byte what;
}

oss_message packet 0x01 {

	blob payload;
}

###############################################################################
###############################################################################

set CMDS(status)	"parse_cmd_status"

proc parse_cmd { line } {

	variable CMDS

	set cc [oss_parse -start $line -skip -return 0]

	if { $cc == "" || $cc == "#" } {
		return ""
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

	$CMDS($cc)
}

proc parse_cmd_status { } {
#
# This is a dummy for now
#
	oss_ttyout "OK"
}

###############################################################################
###############################################################################

proc show_msg { code ref msg } {

	if { $code == 0 } {
		oss_ttyout "OK: $ref"
		return
	}

	set str [oss_getmsgstruct $code name]

	if { $str == "" } {
		error "no user support for the message"
	}

	show_msg_$name $msg
}

proc show_msg_packet { msg } {

	lassign [oss_getvalues $msg "packet"] payload

	set len [llength $payload]

	set rss [lindex $payload end-1]
	set lpq [lindex $payload end]

	set payload [lrange $payload 0 end-2]

	if { $rss == "" } {
		# too short
		return
	}

	set res "PKT: <[format %03d $rss]/$lpq> = "
	append res "[join $payload " " ]"
	oss_ttyout $res
}
