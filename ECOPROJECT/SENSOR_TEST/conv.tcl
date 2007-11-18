#!/bin/tclsh
#
# Temperature/humidity converter
#
while { [gets stdin line] >= 0 } {

	set val [string trim $line]
	if [catch { expr double($val) } val] {
		puts stdout "Sorry ..."
		continue
	}

	set humi [expr $val * 0.0405 - 0.0000028 * $val * $val - 4.0]
	set temp [expr 0.01 * $val - 39.62]

	if { $humi > 100.00 } {
		set humi 100.0
	} elseif { $humi < 0.0 } {
		set humi 0.0
	}

	if { $temp > 100.00 } {
		set temp 100.0
	} elseif { $temp < -40.0 } {
		set temp -40.0
	}



	puts stdout [format "%7.3f %5.2f" $temp $humi]
}
