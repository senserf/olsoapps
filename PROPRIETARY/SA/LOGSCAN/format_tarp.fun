{ bytes } {

##
## Tarp header:
##
##	msg_type	1 byte
##	seq_no		1 byte
##	snd		1 word
##	rcv		1 word
##	hoc		7 bits
##	prox		1 bit
##	hco		7 bits
##	weak		1 bit
##
	set output ""

	set length [llength $bytes]

	if { $length < 10 } {
		return "packet too short"
	}

	set nid [expr { [lindex $bytes 0] | ([lindex $bytes 1] << 8) }]
	set mty [lindex $bytes 2]

	set typ [lindex { "NULL" "PONG" "PACK" "MAST"
			  "REPO" "RACK" "FRWD" "FACK" } $mty]
	if { $typ == "" } {
		set mty [format "  %1x" $mty]
	} else {
		set mty $typ
	}

	set seq [lindex $bytes 3]
	set snd [expr { [lindex $bytes 4] | ([lindex $bytes 5] << 8) }]
	set rcv [expr { [lindex $bytes 6] | ([lindex $bytes 7] << 8) }]
	set hoc [expr { [lindex $bytes 8] & 0x7f }]
	set pro [expr { [lindex $bytes 8] & 0x80 }]
	set hco [expr { [lindex $bytes 9] & 0x7f }]
	set wea [expr { [lindex $bytes 9] & 0x80 }]
	set rss [expr { ([lindex $bytes end-1] + 128) & 0xff }]
	set lqa [expr { [lindex $bytes end] & 0x7f }]
	set chs [expr { [lindex $bytes end] & 0x80 }]

	set bytes [lrange $bytes 10 end-2]

	set output "NID=[format %1x $nid], TYP=$mty, RSS=[format %1d $rss],\
		LQA=[format %1d $lqa], CHS="

	if $chs {
		append output "OK"
	} else {
		append output "BAD"
	}

	append output ", SND=[format %1d $snd], RCV=[format %1d $rcv]"
	append output ", HOC=[format %1d $hoc], HCO=[format %1d $hco], PRO="

	if $pro {
		append output "YES"
	} else {
		append output "NO"
	}

	append output ", WEA="

	if $wea {
		append output "YES"
	} else {
		append output "NO"
	}

	append output ":"

	foreach b $bytes {
		append output [format " %1x" $b]
	}

	return $output
}
