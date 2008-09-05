#!/bin/sh
###########################\
exec tclsh "$0" "$@"

# OSSI module starter

set plist [exec ps x]

if { [string first "ossi.tcl -h server -d data" $plist] >= 0 } {
	puts "OSSI daemon already running"
	exit 0
}

puts "Starting OSSI daemon"

cd /home/econet/COLLECTOR
exec /home/econet/OSSI/ossi.tcl -h server -d data/data -v data/values -x < /dev/null >& /dev/null &
