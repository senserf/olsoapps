#
# Piter I/O preprocessor plugin for the aggregator
#
proc plug_outpp_t { ln } {

	upvar $ln line

	if [regexp "^(1002|2007)" $line] {
		set out [plug_out_sensors $line]
		if { $out == "" } {
			# something wrong
			return 2
		}
		set line $out
		return 1
	}

	# more classifiers ...
	# ...
	# ...

	# line left intact
	return 2
}

proc plug_out_sensors { line } {

	if ![regexp ".*:\[0-9\]+ +(.+)" $line junk vals] {
		# something wrong
		return ""
	}

	set out "$line @@@ "

	set n 0
	while 1 {
		set vals [string trimleft $vals]
		if { $vals == "" } {
			break
		}
		if ![regexp "^(\[^ \t\]+) *(.*)" $vals junk val vals] {
			break
		}
		if [catch { expr $val } val] {
			break
		}
		append out " [plug_conv $n $val],"
		incr n
	}

	return [string trimright $out ","]
}

###############################################################################
### conversion table: sensor position to snippet ##############################
###############################################################################

set plug_ctable {
			plug_conv_int_vol 
			plug_conv_sht_tmp
			plug_conv_sht_hum
			plug_conv_sht_tmp
			plug_conv_sht_hum
			plug_conv_sht_tmp
			plug_conv_sht_hum
			plug_conv_sht_tmp
			plug_conv_sht_hum
			plug_conv_sht_tmp
			plug_conv_sht_hum
		}

proc plug_conv { n v } {

	global plug_ctable

	if { $v < 0 } {
		# absent value
		return "---"
	}

	if { $n >= [llength $plug_ctable] } {
		return "[format %4x $v](raw)"
	}

	return [[lindex $plug_ctable $n] $v]
}

###############################################################################
### conversion "snippets" #####################################################
###############################################################################

proc plug_conv_sht_tmp { v } {

	set v [expr 0.01 * $v - 39.62]
	if { $v > 100.0 } {
		set v 100.0
	} elseif { $v < -40.0 } {
		set v -40.0
	}

	return "[format %6.2f $v](deg)"
}

proc plug_conv_sht_hum { v } {

	set v [expr $v * 0.0405 - 0.0000028 * $v * $v - 4.0]
	if { $v > 100.00 } {
		set v 100.0
	} elseif { $v < 0.0 } {
		set v 0.0
	}

	return "[format %6.2f $v](%)"
}

proc plug_conv_int_vol { v } {

	set v [expr $v * 0.001221]
	return "[format %4.2f $v](V)"
}
