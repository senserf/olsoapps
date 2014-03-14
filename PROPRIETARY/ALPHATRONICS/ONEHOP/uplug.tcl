{

###############################################################################

proc plug_open { nodenum hostnum nodetype tag } {
#
# List: nodenum, hostnum, total, nodetype (txt)
#
	global Plug

	if { $nodetype == "ap321" } {
		set Plug($nodenum,T) $tag
		set Plug($tag,N) $nodenum
		set Plug($tag,S) 0
		term_output "Welcome to the VUEE model of AP321\n\n" $tag
		return 1
	}
	return 0
}

###############################################################################

proc plug_receive { bytes tag hexflag } {

	global Plug

	set msg "RCV:"
	set chunk ""
	while { $bytes != "" } {
		set c [string index $bytes 0]
		set bytes [string range $bytes 1 end]
		scan $c %c v
		lappend chunk $v
		append msg " [format %02x $v]"
	}

	term_output "$msg\n" $tag

	while 1 {

		set bl [llength $chunk]
		if { $bl == 0 } {
			return ""
		}

		switch $Plug($tag,S) {

		0 {
			# waiting for STX
			for { set i 0 } { $i < $bl } { incr i } {
				set c [lindex $chunk $i]
				if { $c == 2 } {
					# STX
					set Plug($tag,S) 1
					set Plug($tag,E) 0
					set Plug($tag,B) ""
					set Plug($tag,L) 0
					# initialize the parity byte
					set Plug($tag,P) 1
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
				if !$Plug($tag,E) {
					if { $c == 3 } {
						# that's it
						set Plug($tag,S) 0
						set chunk [lrange $chunk \
							[expr { $i + 1 }] end]
						if $Plug($tag,P) {
							term_output "Parity\
							    error!" $tag
							break
						}
						# remove the parity byte
						show_message [lrange \
						    $Plug($tag,B) 0 end-1] $tag
						break
					}
					if { $c == 2 } {
						# reset
						set Plug($tag,S) 0
						set chunk [lrange $chunk $i end]
						break
					}
					if { $c == 16 } {
						# escape
						set Plug($tag,E) 1
						continue
					}
				} else {
					set Plug($tag,E) 0
				}
				lappend Plug($tag,B) $c
				set Plug($tag,P) [expr { $Plug($tag,P) ^ $c }]
				incr Plug($tag,L)
			}
			if $Plug($tag,S) {
				set chunk ""
			}
		}

		}
	}
}

proc show_message { lbytes tag } {

	set ll [llength $lbytes]
	if { [llength $lbytes] < 13 } {
		term_output "Message too short, $ll bytes\n" $tag
		return
	}

	term_output "TP LN CurrNd Sender But Glb Tst Vlt RSS XPW ADD\n" $tag

	set msg ""
	append msg "[format %02X [lindex $lbytes 0]]"
	append msg " [format %2d [lindex $lbytes 1]]"
	append msg " [format %6d [expr { [lindex $lbytes 2] | \
			 ([lindex $lbytes 3] << 8) }]]"
	append msg " [format %6d [expr { [lindex $lbytes 4] | \
			 ([lindex $lbytes 5] << 8) }]]"
	append msg " [format %3d [lindex $lbytes 6]]"
	append msg " [format %3d [lindex $lbytes 7]]"
	append msg " [format %3d [lindex $lbytes 8]]"
	append msg " [format %3d [lindex $lbytes 9]]"
	append msg " [format %3d [lindex $lbytes 10]]"
	append msg " [format %3d [lindex $lbytes 11]]"
	append msg " [format %3d [lindex $lbytes 12]]"

	term_output "$msg\n\n" $tag
}

proc plug_input { tx tag } {

	upvar $tx text

	term_output "GOT: $text\n" $tag

	return 1
}

proc plug_close { tag } {

	global Plug

	if [info exists Plug($tag,N)] {
		set n $Plug($tag,N)
		unset Plug($n,T)
		array unset Plug "$tag,*"
	}
}

###############################################################################

}
