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
		set tty [format "  %1x" $mty]
	} else {
		set tty $typ
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
	set length [expr { $length - 12 }]

	set output "NID=[format %1x $nid], TYP=$tty, SEQ=$seq,\
		RSS=[format %1d $rss], LQA=[format %1d $lqa], CHS="

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

	if $chs {
		# do content-specific formatting only if the checksum is OK
		if { ($mty == 4 && $length >= 12) ||
		    ($mty == 1 && $length >= 6) } {
			# REPO/PONG
			if { $mty == 4 } {
				set ref [expr { [lindex $bytes 0] |
					([lindex $bytes 1] << 8) }]
				set tag [expr { [lindex $bytes 2] |
					([lindex $bytes 3] << 8) }]
				set rss [expr { [lindex $bytes 4] }]
				set ago [expr { [lindex $bytes 5] }]
				append output " REF=$ref, TAG=$tag, RSS=$rss,\
					AGO=$ago,"
				set bytes [lrange $bytes 6 end]
				set length [expr { $length - 6 }]
			}

			set w 	[expr { [lindex $bytes 0] |
				([lindex $bytes 1] << 8) }]
			set btn [expr { $w & 0xf }]
			set bty [lindex \
				{ "CHR" "CHW" "PEG" "6BU" "1BU" "WAR" } $btn]
			if { $bty == "" } {
				set bty $btn
			}
			set pow [expr { ($w >> 4) & 7 }]
			set alr [expr { ($w >> 7) & 7 }]
			set asq [expr { ($w >> 10) & 0xf }]
			set fl2 [expr { ($w >> 14) & 0x3 }]
			set w 	[expr { [lindex $bytes 2] |
				([lindex $bytes 3] << 8) }]
			set len [expr { ($w & 0x3f) }]
			set try [expr { ($w >> 6) & 0x3 }]
			set dpq [expr { ($w >> 9) & 0xf }]
			set noa [expr { ($w >> 13) & 0x1 }]
			if $noa {
				set noa "YES"
			} else {
				set noa "NO"
			}
			set vol [expr { [lindex $bytes 4] |
				([lindex $bytes 5] << 8) }]

			append output " BTY=$bty, POW=$pow, ALM=$alr, ASQ=$asq,\
				FL2=$fl2, LEN=$len, TRY=$try, DPQ=$dpq,\
				NOA=$noa, VOL=$vol"

			set bytes [lrange $bytes 6 end]
			set length [expr { $length - 6 }]
			if { $length >= 4 && ($btn == 0 || $btn == 1) } {
				# chronos
				set mva [expr { [lindex $bytes 0] |
					([lindex $bytes 1] << 8) }]
				set mvc [expr { [lindex $bytes 2] |
					([lindex $bytes 3] << 8) }]
				append output ", MVA=$mva, MVC=$mvc"
				set bytes [lrange $bytes 4 end]
			} elseif { $length >= 2 && $btn == 3 } {
				set w 	[expr { [lindex $bytes 0] |
					([lindex $bytes 1] << 8) }]
				set dia [expr { $w & 0xff }]
				set glo [expr { ($w >> 8) & 0x1 }]
				if $glo {
					set glo "YES"
				} else {
					set glo "NO"
				}
				append output ", DIA=$dia, GLO=$glo"
				set bytes [lrange $bytes 2 end]
			} elseif { $length >= 4 && $btn == 5 } {
				# warsaw
				set rnd [expr { [lindex $bytes 0] |
					([lindex $bytes 1] << 8) }]
				set sta [expr { [lindex $bytes 2] |
					([lindex $bytes 3] << 8) }]
				append output ", RND=$rnd, STA=$sta"
				set bytes [lrange $bytes 4 end]
			}
			if { $bytes != "" } {
				append output ":"
			}
		}

		if { $mty == 5 && $length >= 4 } {
			# REPO
			set ref [expr { [lindex $bytes 0] |
				([lindex $bytes 1] << 8) }]
			set tag [expr { [lindex $bytes 2] |
				([lindex $bytes 3] << 8) }]

			append output " REF=$ref, TAG=$tag"
			set bytes [lrange $bytes 4 end]
			if { $bytes != "" } {
				append output ":"
			}
		}

		if { $mty == 2 && $length >= 2 } {
			# PACK
			set w   [expr { [lindex $bytes 0] |
				([lindex $bytes 1] << 8) }]
			set dpq [expr { $w & 0xf }]
			append output " DPQ=$dpq"
			set bytes [lrange $bytes 2 end]
			if { $bytes != "" } {
				append output ":"
			}
		}

	}

	foreach b $bytes {
		append output [format " %1x" $b]
	}

	return $output
}
