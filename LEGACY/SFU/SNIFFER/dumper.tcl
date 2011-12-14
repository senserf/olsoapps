#!/usr/bin/tclsh
################

array set CMD		{ BIND 0 UNBIND 64 RESET 16 STOP 32 ABORT 33 REPORT 48
				SAMPLE 80 SAMPLEX 81 HRMON 112 HRMOFF 113
					SEND 96 HELLO 128 HRATE 208 SDATA 192
							STATUS 144 }
proc get { n } {

	global LEN VAL

	if { $LEN < $n } {
		set LEN 0
		return 0
	}

	set LEN [expr $LEN - $n]

	set val 0
	set cnt $n

	while 1 {
		incr cnt -1
		set val [expr ($val << 8) | 0x[lindex $VAL $cnt]]
		if { $cnt == 0 } {
			break
		}
	}

	set VAL [lrange $VAL $n end]

	return $val
}
		
proc parse { } {

	global LEN CMD

	set lid [get 2]
	set cmd [get 1]

	if { [expr $cmd & 0x80] } {
	    set OUT "N->A L[format %04x $lid]: "
	    if { [expr ($cmd & 0xf0)] == $CMD(STATUS) } {
		append OUT "STATUS "
		set st [expr ($cmd >> 1) & 0x3]
		set mo [expr $cmd & 0x1]
		if $mo {
			append OUT "*"
		} else {
			append OUT "-"
		}
		if { $st == 0 } {
			# ready
			append OUT "READY "
			# determine the number of samples
			set ns [expr $LEN / 4]
			if { $ns == 0 } {
				append OUT "\[no samples\]"
			} else {
				while { $ns } {
					set si [get 1]
					set sl [get 3]
					append OUT "\[$si, $sl\] "
					incr ns -1
				}
			}
		} elseif { $st == 1 } {
			# sampling
			append OUT "SAMPLING "
			set si [get 1]
			set sl [get 3]
			append OUT "\[$si, $sl\]"
		} elseif { $st == 2 } {
			# sending
			append OUT "SENDING "
			set si [get 1]
			set sl [get 3]
			append OUT "\[$si, $sl\]"
		} else {
			# garbage
			append OUT "UNKNOWN "
		}
	    } elseif { $cmd == $CMD(HELLO) } {
		set sl [get 4]
		append OUT "HELLO '$sl'"
	    } elseif { $cmd == $CMD(HRATE) } {
		append OUT "HRATE <[get 1]>"
	    } elseif { $cmd == $CMD(SDATA) } {
		set si [get 1]
		set hr [get 1]
		if { $LEN < 51 } {
			# EOR
			append OUT "EOR \[$si\] <$hr>"
		} else {
			set sl [get 3]
			append OUT "SDATA \[$si, $sl\] <$hr>"
		}
	    } else {
		append OUT "UNKNOWN"
	    }
	} else {

	    set OUT "A->N L[format %04x $lid]: "

	    if { $cmd == $CMD(BIND) } {
		set sl [get 4]
		append OUT "BIND '$sl'"
	    } elseif { $cmd == $CMD(UNBIND) } {
		append OUT "UNBIND"
	    } elseif { $cmd == $CMD(RESET) } {
		append OUT "RESET"
	    } elseif { $cmd == $CMD(STOP) } {
		append OUT "STOP"
	    } elseif { $cmd == $CMD(REPORT) } {
		append OUT "REPORT"
	    } elseif { $cmd == $CMD(SAMPLE) || $cmd == $CMD(SAMPLEX) } {
		append OUT "SAMPLE"
		if { $cmd == $CMD(SAMPLEX) } {
			append OUT "&SEND"
		}
		set si [get 1]
		set sl [get 3]
		append OUT " \[$si, $sl\]"
	    } elseif { $cmd == $CMD(SEND) } {
		set si [get 1]
		set n [expr $LEN / 3]
		append OUT "SEND \[$si, @$n\]"
	    } elseif { $cmd == $CMD(HRMON) } {
		append OUT "HRM ON"
	    } elseif { $cmd == $CMD(HRMOFF) } {
		append OUT "HRM OFF"
	    } else {
		append OUT "UNKNOWN"
	    }
	}

	puts " $OUT"
}

######### COM port ############################################################

set cpn [lindex $argv 0]

if { $cpn == "" } {
	# COM 1 is the default
	set cpn 1
}

if [catch { open "com${cpn}:" "r+" } SFD] {
	# try tty
	if [catch { open "/dev/ttyUSB${cpn}" "r+" } SFD] {
		# try ttyUSB
		if [catch { open "/dev/tty${cpn}" "r+" } SFD] {
			puts "Cannot open input device"
			exit 1
		}
	}
}

unset cpn

fconfigure $SFD -mode "115200,n,8,1" -handshake none

###############################################################################

while 1 {

	set line [gets $SFD]


	if ![regexp "^(\[0-9\]\[0-9\]): +(.+)" $line junk LEN VAL] {
		# ignore garbage
		continue
	}

	puts "\[$line\]"

	if { [string index $LEN 0] == "0" } {
		# avoid interpretation as octal
		set LEN [string index $LEN 1]
	}

	set VAL [regexp -all -inline -nocase "\[0-9a-f\]\[0-9a-f\]" $VAL]

	if { $LEN < 4 || $LEN != [llength $VAL] } {
		continue
	}

	parse
}
