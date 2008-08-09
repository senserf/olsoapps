#!/bin/sh
###########################\
exec tclsh "$0" "$@"

###############################################################################

proc abt { msg } {

	puts stderr $msg
	exit 0
}

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

###############################################################################

#
# UART functions
#

proc abinS { s h } {
#
# append one short int to string s (in network order)
#
        upvar $s str
        append str [binary format S $h]
}

proc abinI { s l } {
#
# append one 32-bit int to string s (in network order)
#
        upvar $s str
        append str [binary format I $l]
}

proc dbinB { s } {
#
# decode one binary byte from string s
#
        upvar $s str
        if { $str == "" } {
                return -1
        }
        binary scan $str c val
        set str [string range $str 1 end]
        return [expr ($val & 0x000000ff)]
}

proc uart_tmout { } {

        global Turn Uart

        catch { close $Uart(TS) }
        set Uart(TS) ""
        incr Turn
}

proc uart_ctmout { } {

        global Uart

        if [info exists Uart(TO)] {
                after cancel $Uart(TO)
                unset Uart(TO)
        }
}

proc uart_stmout { del fun } {

        global Uart

        if [info exists Uart(TO)] {
                # cancel the previous timeout
                uart_ctmout
        }

        set Uart(TO) [after $del $fun]
}

proc uart_sokin { } {
#
# Initial read: VUEE handshake
#
        global Uart Turn

        uart_ctmout

        if { [catch { read $Uart(TS) 1 } res] || $res == "" } {
                # disconnection
                catch { close $Uart(TS) }
                set Uart(TS) ""
                incr Turn
                return
        }

        set code [dbinB res]

        if { $code != 129 } {
                catch { close $Uart(TS) }
                set Uart(TS) ""
                incr Turn
                return
        }

        # so far, so good
        incr Turn
}

proc uart_incoming { sok h p } {
#
# Incoming connection
#
        global Uart Turn

        if { $Uart(FD) != "" } {
                # connection already in progress
                msg "incoming connection from $h:$p ignored, already connected"
                catch { close $sok }
                return
        }

        if [catch { fconfigure $sok -blocking 0 -buffering none -translation \
            binary -encoding binary } err] {
                msg "cannot configure socket for incoming connection: $err"
                set Uart(FD) ""
                return
        }

        set Uart(FD) $sok
        set Uart(BF) ""
        fileevent $sok readable "uart_read"
        incr Turn
}

proc uart_init { rfun } {

        global Uart Turn

        set Uart(RF) $rfun

        if { [info exists Uart(FD)] && $Uart(FD) != "" } {
                catch { close $Uart(FD) }
                unset Uart(FD)
        }

        if { $Uart(MODE) == 0 } {

                # straightforward UART
                msg "connecting to UART $Uart(DEV), encoding $Uart(PAR) ..."
                # try a regular UART
                if [catch { open $Uart(DEV) RDWR } ser] {
                        abt "cannot open UART $Uart(DEV): $ser"
                }
                if [catch { fconfigure $ser -mode $Uart(PAR) -handshake none \
                        -blocking 0 -translation binary } err] {
                        abt "cannot configure UART $Uart(DEV)/$Uart(PAR): $err"
                }
                set Uart(FD) $ser
                set Uart(BF) ""
                fileevent $Uart(FD) readable "uart_read"
                incr Turn
                return
        }

        if { $Uart(MODE) == 1 } {

                # VUEE: a single socket connection
                msg "connecting to a VUEE model: node $Uart(NODE),\                                     host $Uart(HOST), port $Uart(PORT) ..."

                if [catch { socket -async $Uart(HOST) $Uart(PORT) } ser] {
                        abt "connection failed: $ser"
                }

                if [catch { fconfigure $ser -blocking 0 -buffering none \
                    -translation binary -encoding binary } err] {
                        abt "connection failed: $err"
                }

                set Uart(TS) $ser

                # send the request
                set rqs ""
                abinS rqs 0xBAB4

                abinS rqs 1
                abinI rqs $Uart(NODE)

                if [catch { puts -nonewline $ser $rqs } err] {
                        abt "connection failed: $err"
                }

                for { set i 0 } { $i < 10 } { incr i } {
                        if [catch { flush $ser } err] {
                                abt "connection failed: $err"
                        }
                        if ![fblocked $ser] {
                                break
                        }
                        after 1000
                }

                if { $i == 10 } {
                        abt "Timeout"
                }

                catch { flush $ser }

                # wait for a reply
                fileevent $ser readable "uart_sokin"
                uart_stmout 10000 uart_tmout
                vwait Turn

                if { $Uart(TS) == "" } {
                        abt "connection failed: timeout"
                }
                set Uart(FD) $Uart(TS)
                unset Uart(TS)

                set Uart(BF) ""
                fileevent $Uart(FD) readable "uart_read"
                incr Turn
                return
        }

        # server

        msg "setting up server socket on port $Uart(PORT) ..."
        if [catch { socket -server uart_incoming $Uart(PORT) } ser] {
                abt "cannot set up server socket: $ser"
        }

        # wait for connections: Uart(FD) == ""
}

proc uart_read { } {

        global Uart

        if $Uart(MODE) {

                # socket

                if [catch { read $Uart(FD) } chunk] {
                        # disconnection
                        msg "connection broken by peer: $chunk"
                        catch { close $Uart(FD) }
                        set Uart(FD) ""
                        return
                }

                if [eof $Uart(FD)] {
                        msg "connection closed by peer"
                        catch { close $Uart(FD) }
                        set Uart(FD) ""
                        return
                }

        } else {

                # regular UART

                if [catch { read $Uart(FD) } chunk] {
                        # ignore errors
                        return
                }
        }

        if { $chunk == "" } {
                # nothing available
                return
        }

        uart_ctmout

        append Uart(BF) $chunk

        while 1 {
                set el [string first "\n" $Uart(BF)]
                if { $el < 0 } {
                        break
                }
                set ln [string range $Uart(BF) 0 $el]
                incr el
                set Uart(BF) [string range $Uart(BF) $el end]
                $Uart(RF) $ln
        }

        # if the buffer is nonempty, erase it on timeout

        if { $Uart(BF) != "" } {
                uart_stmout 1000 uart_erase
        }
}

proc uart_erase { } {

        global Uart

        unset Uart(TO)
        set Uart(BF) ""
}

proc uart_write { w } {

        global Uart

        msg "-> $w"

        if [catch {
                puts -nonewline $Uart(FD) "$w\r\n"
                flush $Uart(FD)
        } err] {
                if $Uart(MODE) {
                        msg "connection closed by peer"
                        catch { close $Uart(FD) }
                        set Uart(FD) ""
                }
        }
}

###############################################################################

proc input_line { inp } {
#
# Receive a line from UART
#
	set inp [string trimright $inp]
	# log $inp
	# echo it
	puts $inp
	# check if this is a relevant line
	if [regexp "^ (\[0-9\]+) (\[0-9\]+) (\[0-9\]+) (\[0-9\]+) (\[0-9\]+) (\[0-9\]+) (\[0-9\]+) (\[0-9\]+) (\[0-9\]+) (\[0-9\]+) (\[0-9\]+) (\[0-9\]+) (\[0-9\]+) (\[0-9\]+) (\[0-9\]+) (\[0-9\]+) (\[0-9\]+) (\[0-9\]+) (\[0-9\]+)$" \
		$inp j0 j1 pa0 pa1 pa2 rsa j2 fa0 fa1 ta0 ta1 pb0 pb1 pb2 rsb \
		j3 fb0 fb1 tb0 tb1] {

		process_sample $pa0 $pa1 $pa2 $fa0 $fa1 $ta0 $ta1 $rsa
		process_sample $pb0 $pb1 $pb2 $fb0 $fb1 $tb0 $tb1 $rsb
	}
}

proc process_sample { p0 p1 p2 f0 f1 t0 t1 rssi } {
#
# Send it to a file for now
#
	global SFD

	puts $SFD "ENV = ($p0, $p1, $p2) FROM ($f0, $f1) TO ($t0, $t1) : $rssi"
}

proc stdin_init { } {
#
# Event-driven input from stdio
#
	fconfigure stdin -blocking 0 -buffering line
        fileevent stdin readable stdin_line
}

proc stdin_line { } {
#
	set line [string trim [gets stdin]]
	if { $line == "" } {
		return
	}

	if { [string range $line 0 2] == "!e" } {
		exit 0
	}
	# send it as input to the device
	uart_write $line
}

proc msg { m } {

	puts $m
	log $m
}

set Uart(DEV)	"/dev/ttyUSB0"
set Uart(PAR)	"19200,n,8,1"
set Uart(MODE)	0

set Files(LOG)		"log"
set Log(MAXSIZE)	1000000
set Turn		0

log_open
uart_init input_line
stdin_init

set SFD [open sample_file "w"]

while 1 {
	vwait Turn
}
