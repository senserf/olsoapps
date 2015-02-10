oss_interface -id 0x00010007 -speed 115200 -length 82 \
	-parser { parse_cmd show_msg }

###############################################################################
###############################################################################

oss_command packet 0x01 {
#
# This is a dummy at present
#
	blob payload;
}

oss_message packet 0x01 {

	blob payload;
}

###############################################################################
###############################################################################

set CMDS(status)	"parse_cmd_status"
set last_day		""

proc parse_cmd { line { rec 0 } } {

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

	if { $rec == 0 && $cc == "@" } {
		# a file
		set cc [oss_parse -skip -match {^[^[:blank:]]+} -return 1]
		if { $cc == "" } {
			error "file name expected"
		}
		# try to open the file
		if [catch { open $cc "r" } fd] {
			error "cannot open file $cc, $fd"
		}
		if [catch { read $fd } cmds] {
			catch { close $fd }
			error "cannot read file $cc, $cmds"
		}
		catch { close $fd }
		set cmds [split $cmds "\n"]
		foreach ln $cmds {
			if [catch { parse_cmd $ln 1 } err] {
				error "file processing aborted, $err"
			}
		}
		oss_ttyout "file processing complete"
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

	variable last_day

	lassign [oss_getvalues $msg "packet"] pkt

	set sec [clock seconds]
	set day [clock format $sec -format %d]
	set hdr [clock format $sec -format "%H:%M:%S"]

	if { $day != $last_day } {
		if { $last_day != "" } {
			set res "00:00:00 #### BIM! BOM! "
		} else {
			set res "$hdr #### "
		}
		append res "Today is "
		append res [clock format $sec -format "%h $day, %Y"]
		set last_day $day
		oss_ttyout $res
	}

	set res "$hdr <="

	foreach b $pkt {
		# strip 0x
		append res " [string range $b 2 3]"
	}

	oss_ttyout $res
}
