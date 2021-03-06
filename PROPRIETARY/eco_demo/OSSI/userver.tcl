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


set Files(LOG)		"userver_log"
set Log(MAXSIZE)	1000000
set Log(MAXVERS)	4

###############################################################################

#
# Log functions
#

proc log_open { } {

	global Files Log

	if [info exists Log(FD)] {
		# close previous log
		catch { close $Log(FD) }
		unset Log(FD)
	}

	if [catch { file size $Files(LOG) } fs] {
		# not present
		if [catch { open $Files(LOG) "w" } fd] {
			abt "Cannot open log: $fd"
		}
		# empty log
		set Log(SIZE) 0
	} else {
		# log file exists
		if [catch { open $Files(LOG) "a" } fd] {
			abt "Cannot open log: $fd"
		}
		set Log(SIZE) $fs
	}
	set Log(FD) $fd
	set Log(CD) 0
}

proc log_rotate { } {

	global Files Log

	catch { close $Log(FD) }
	unset Log(FD)

	for { set i $Log(MAXVERS) } { $i > 0 } { incr i -1 } {
		set tfn "$Files(LOG).$i"
		set ofn $Files(LOG)
		if { $i > 1 } {
			append ofn ".[expr $i - 1]"
		}
		catch { file rename -force $ofn $tfn }
	}

	log_open
}

proc log_out { m } {

	global Log

	catch {
		puts $Log(FD) $m
		flush $Log(FD)
	}

	incr Log(SIZE) [string length $m]
	incr Log(SIZE)

	if { $Log(SIZE) >= $Log(MAXSIZE) } {
		log_rotate
	}
}

proc log { m } {

	global Log 

	if ![info exists Log(FD)] {
		# no log filr
		return
	}

	set sec [clock seconds]
	set day [clock format $sec -format %d]
	set hdr [clock format $sec -format "%H:%M:%S"]

	if { $day != $Log(CD) } {
		# day change
		set today "Today is "
		append today [clock format $sec -format "%h $day, %Y"]
		if { $Log(CD) == 0 } {
			# startup
			log_out "$hdr #### $today ####"
		} else {
			log_out "00:00:00 #### BIM! BOM! $today ####"
		}
		set Log(CD) $day
	}

	log_out "$hdr $m"
}

proc msg { m } {

	puts $m
	log $m
}

###############################################################################

proc napin { n } {
#
# Not A Positive Integer Number
#
	upvar $n num

	if [catch { expr $num } num] {
		return 1
	}
	set t $num
	if [catch { incr t }] {
		return 1
	}
	if { $num <= 0 } {
		return 1
	}
	return 0
}

proc open_uart { } {
#
	global Uart

	catch { close $Uart(DE) }

	# try to make sure the permissions are right

	if [regexp "^/dev/tty(.*)" $Uart(DEV) junk suf] {
		msg "changing permissions on $suf"
		if [catch { exec owntty $suf } err] {
			msg "failed to change permissions: $err"
		}
	}

	if [catch { open $Uart(DEV) RDWR } ser] {
		msg "cannot open UART $Uart(DEV): $ser"
		return 0
	}

	if [catch { fconfigure $ser -mode $Uart(PAR) -handshake none \
		-buffering none -blocking 0 -translation { auto crlf } } err] {
		msg "cannot configure UART $Uart(DEV)/$Uart(PAR): $err"
		return 0
	}
	set Uart(DE) $ser
	return 1
}

proc establish_connection { } {
#
	global Uart
	
	msg "trying to connect to $Uart(HOST):$Uart(PORT)"

	if [catch { socket $Uart(HOST) $Uart(PORT) } sok] {
		msg "connection failed: $sok"
		# we shall retry in 5 secs
		return
	}

	if [catch { fconfigure $sok -blocking 0 -buffering none \
	    -translation auto -encoding binary } err] {
		msg "socket configuration failed: $err"
		return
	}

	set Uart(SO) $sok

	# send a dummy line
	if [catch { puts $sok "0000 hello" } err] {
		msg "socket write failed: $err"
		return
	}

	msg "connected"

	set Uart(MORE) 1
	set Uart(BUF) ""
	set Uart(LS) 0

	# wait for events
	fileevent $Uart(SO) readable "uart_sokin"
	fileevent $Uart(DE) readable "uart_devin"

	while 1 {
		vwait Uart(MORE)
		if { $Uart(MORE) == 0 } {
			# disconnected
			return
		}
		uart_send
	}
}

proc uart_send { } {
#
# Send a line to the master
#
	global Uart

	# check if there is a line pending

	set ix [string first "\n" $Uart(BUF)]
	if { $ix < 0 } {
		return
	}

	set tim [clock seconds]
	if { $tim == $Uart(LS) } {
		# no more than one line per second
		set bs [string length $Uart(BUF)]
		# drop lines if there's a lot
		if { $bs > 256 } {
			while 1 {
				incr ix
				set Uart(BUF) [string range $Uart(BUF) $ix end]
				set bs [expr $bs - $ix]
				if { $bs <= 128 } {
					return
				}
				set ix [string first "\n" $Uart(BUF)]
				if { $ix < 0 } {
					return
				}
			}
		}
		return
	}

	# extract one line

	set line [string trim [string range $Uart(BUF) 0 $ix]]
	incr ix
	set Uart(BUF) [string range $Uart(BUF) $ix end]

	# and write it to the UART
	set Uart(LS) $tim

	# ignore UART errors
	msg "-> $line"
	catch { puts $Uart(DE) $line }
	catch { flush $Uart(DE) }
}

proc uart_sokin { } {
#
# Socket input
#
	global Uart

	if [catch { read $Uart(SO) } res] {
		# disconnection
		catch { close $Uart(SO) }
		set Uart(SO) ""
		msg "broken connection: $res"
		set Uart(MORE) 0
		return
	}

	if [eof $Uart(SO)] {
		catch { close $Uart(SO) }
		set Uart(SO) ""
		msg "connection closed by peer"
		set Uart(MORE) 0
		return
	}

	append Uart(BUF) $res
	set Uart(MORE) 1
}

proc uart_devin { } {
#
# Uart input
#
	global Uart

	if [catch { read $Uart(DE) } res] {
		# ignore UART errors
		return
	}

	if { $res == "" } {
		return
	}

	if { $Uart(SO) == "" } {
		return
	}

	if [catch { puts -nonewline $Uart(SO) $res } err] {
		# connection closed
		catch { close $Uart(SO) }
		set Uart(SO) ""
		msg "connection closed, write failed: $err"
		set Uart(MORE) 0
	}
}

#
# Process the arguments
#

proc bad_usage { } {

	global argv0

	puts "Usage: $argv0 options, where options can be:\n"
	puts "       -u uart_device, no default, required"
	puts "       -h hostname, default is localhost"
	puts "       -p port, default is 4445"
	puts "       -e uart_encoding_params, default is 19200,n,8,1"
	puts ""
	exit 99
}

while { $argv != "" } {

	set fg [lindex $argv 0]
	set argv [lrange $argv 1 end]
	set va [lindex $argv 0]
	if { $va == "" || [string index $va 0] == "-" } {
		set va ""
	} else {
		set argv [lrange $argv 1 end]
	}

	if { $va == "" } {
		bad_usage
	}

	if { $fg == "-h" } {
		if [info exists Uart(HOST)] {
			bad_usage
		}
		set Uart(HOST) $va
		continue
	}

	if { $fg == "-p" } {
		if [info exists Uart(PORT)] {
			bad_usage
		}
		if { [napin va] || $va > 65535 } {
			bad_usage
		}
		set Uart(PORT) $va
		continue
	}

	if { $fg == "-u" } {
		if [info exists Uart(DEV)] {
			bad_usage
		}
		set Uart(DEV) $va
		continue
	}

	if { $fg == "-e" } {
		if [info exists Uart(PAR)] {
			bad_usage
		}
		set Uart(PAR) $va
		continue
	}
}

if ![info exists Uart(HOST)] {
	set Uart(HOST) "localhost"
}

if ![info exists Uart(PORT)] {
	set Uart(PORT) 4445
}

if ![info exists Uart(DEV)] {
	bad_usage
}

if ![info exists Uart(PAR)] {
	set Uart(PAR) "19200,n,8,1"
}

set Uart(SO) ""

log_open

while 1 {

	# persistently try to re-open the UART
	if [open_uart] {

		# persistently try to establish connection
		catch { establish_connection }
	}

	# will only return if there was a failure
	after 5000
}
