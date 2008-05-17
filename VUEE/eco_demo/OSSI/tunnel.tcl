#!/bin/sh
###########################\
exec tclsh "$0" "$@"

set Files(LOG)	"tunnel_log"
set Log(MAXSIZE) 1000000
set Log(MAXVERS) 4

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
proc bad_usage { } {

	global argv0

	puts "Usage: $argv0 options, where options can be:\n"
	puts "       -h hostname, required"
	puts "       -u user, default is same as local user"
	puts "       -p local port, default is 4445"
	puts "       -r remote port, default is same as local"
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
		if [info exists Net(HOST)] {
			bad_usage
		}
		set Net(HOST) $va
		continue
	}

	if { $fg == "-u" } {
		if [info exists Net(USER)] {
			bad_usage
		}
		set Net(USER) $va
		continue
	}

	if { $fg == "-p" } {
		if [info exists Net(PORT)] {
			bad_usage
		}
		if { [napin va] || $va > 65535 } {
			bad_usage
		}
		set Net(PORT) $va
		continue
	}

	if { $fg == "-r" } {
		if [info exists Net(REMOTE)] {
			bad_usage
		}
		if { [napin va] || $va > 65535 } {
			bad_usage
		}
		set Net(REMOTE) $va
		continue
	}
}

if ![info exists Net(HOST)] {
	bad_usage
}

if ![info exists Net(PORT)] {
	set Net(PORT) 4445
}

if ![info exists Net(REMOTE)] {
	set Net(REMOTE) $Net(PORT)
}

if ![info exists Net(USER)] {
	set um " (local user)"
} else {
	set um ", user $Net(USER)"
}

log_open

msg "started tunnel monitoring: $Net(PORT) -> $Net(HOST):$Net(REMOTE)$um"
set SO ""

while 1 {

	catch { exec killall -9 "ssh" }
	after 2000

	if [catch {

	    msg "connecting ..."
	    if ![info exists Net(USER)] {

		set SO [exec ssh $Net(HOST) -f -N -T -n -L \
					$Net(PORT):$Net(HOST):$Net(REMOTE)]

	    } else {

		set SO [exec ssh $Net(HOST) -f -N -T -n -l $Net(USER) \
			-L $Net(PORT):$Net(HOST):$Net(REMOTE)]
	    }
	} err] {
		msg "failed: $err $SO"
		after 10000
	} else {
		msg "dropped: $SO"
		after 5000
	}
}
