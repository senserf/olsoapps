#!/bin/sh
#
#	Copyright 2002-2020 (C) Olsonet Communications Corporation
#	Programmed by Pawel Gburzynski & Wlodek Olesinski
#	All rights reserved
#
#	This file is part of the PICOS platform
#
#
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
