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


##################################################################
##################################################################

#
# Removed msource as being a pain in the ass; the "packages" are simply copied
# here from PICOS/Scripts. Any way to make it both convenient and reliable does
# not work, unless we want to make it unnecessarily "serious".
#

package provide xml 1.0

namespace eval XML {

proc xstring { s } {
#
# Extract a possibly quoted string
#
	upvar $s str

	if { [xspace str] != "" } {
		error "illegal white space"
	}

	set c [string index $str 0]
	if { $c == "" } {
		error "empty string illegal"
	}

	if { $c != "\"" } {
		# no quote; this is formally illegal in XML, but let's be
		# pragmatic
		regexp "^\[^ \t\n\r\>\]+" $str val
		set str [string range $str [string length $val] end]
		return [xunesc $val]
	}

	# the tricky way
	if ![regexp "^.(\[^\"\]*)\"" $str match val] {
		error "missing \" in string"
	}
	set str [string range $str [string length $match] end]

	return [xunesc $val]
}

proc xunesc { str } {
#
# Remove escapes from text
#
	regsub -all "&amp;" $str "\\&" str
	regsub -all "&quot;" $str "\"" str
	regsub -all "&lt;" $str "<" str
	regsub -all "&gt;" $str ">" str
	regsub -all "&nbsp;" $str " " str

	return $str
}

proc xspace { s } {
#
# Skip white space
#
	upvar $s str

	if [regexp -indices "^\[ \t\r\n\]+" $str ix] {
		set ix [lindex $ix 1]
		set match [string range $str 0 $ix]
		set str [string range $str [expr $ix + 1] end]
		return $match
	}

	return ""
}

proc xcmnt { s } {
#
# Skip a comment
#
	upvar $s str

	set sav $str

	set str [string range $str 4 end]
	set cnt 1

	while 1 {
		set ix [string first "-->" $str]
		set iy [string first "<!--" $str]
		if { $ix < 0 } {
			error "unterminated comment: [string range $sav 0 15]"
		}
		if { $iy > 0 && $iy < $ix } {
			incr cnt
			set str [string range $str [expr $iy + 4] end]
		} else {
			set str [string range $str [expr $ix + 3] end]
			incr cnt -1
			if { $cnt == 0 } {
				return
			}
		}
	}
}

proc xftag { s } {
#
# Find and extract the first tag in the string
#
	upvar $s str

	set front ""

	while 1 {
		# locate the first tag
		set ix [string first "<" $str]
		if { $ix < 0 } {
			set str "$front$str"
			return ""
		}
		append front [string range $str 0 [expr $ix - 1]]
		set str [string range $str $ix end]
		# check for a comment
		if { [string range $str 0 3] == "<!--" } {
			# skip the comment
			xcmnt str
			continue
		}
		set et ""
		if [regexp -nocase "^<(/)?\[a-z:_\]" $str ix et] {
			# this is a tag
			break
		}
		# skip the thing and keep going
		append front "<"
		set str [string range $str 1 end]
	}

	if { $et != "" } {
		set tm 1
	} else {
		set tm 0
	}

	if { $et != "" } {
		# terminator, skip the '/', so the text is positioned at the
		# beginning of keyword
		set ix 2
	} else {
		set ix 1
	}

	# starting at the keyword
	set str [string range $str $ix end]

	if ![regexp -nocase "^(\[a-z0-9:_\]+)(.*)" $str ix kwd str] {
		# error
		error "illegal tag: [string range $str 0 15]"
	}

	set kwd [string tolower $kwd]

	# decode any attributes
	set attr ""
	array unset atts

	while 1 {
		xspace str
		if { $str == "" } {
			error "unterminated tag: <$et$kwd"
		}
		set c [string index $str 0]
		if { $c == "/" } {
			# self-terminating
			if { $tm != 0 || [string index $str 1] != ">" } {
				error "broken self-terminating tag:\
					<$et$kwd ... [string range $str 0 15]"
			}
			set str [string range $str 2 end]
			return [list 2 $front $kwd $attr]
		}
		if { $c == ">" } {
			# done
			set str [string range $str 1 end]
			# term preceding_text keyword attributes
			return [list $tm $front $kwd $attr]
		}
		# this must be a keyword
		if ![regexp -nocase "^(\[a-z\]\[a-z0-9_\]*)=" $str match atr] {
			error "illegal attribute: <$et$kwd ... [string range \
				$str 0 15]"
		}
		set atr [string tolower $atr]
		if [info exists atts($attr)] {
			error "duplicate attribute: <$et$kwd ... $atr"
		}
		set atts($atr) ""
		set str [string range $str [string length $match] end]
		if [catch { xstring str } val] {
			error "illegal attribute value: \
				<$et$kwd ... $atr=[string range $str 0 15]"
		}
		lappend attr [list $atr $val]
	}
}

proc xadv { s kwd } {
#
# Returns the text + the list of children for the current tag
#
	upvar $s str

	set txt ""
	set chd ""

	while 1 {
		# locate the nearest tag
		set tag [xftag str]
		if { $tag == "" } {
			# no more
			if { $kwd != "" } {
				error "unterminated tag: <$kwd ...>"
			}
			return [list "$txt$str" $chd]
		}

		set md [lindex $tag 0]
		set fr [lindex $tag 1]
		set kw [lindex $tag 2]
		set at [lindex $tag 3]

		append txt $fr

		if { $md == 0 } {
			# opening, not self-closing
			set cl [xadv str $kw]
			# inclusion ?
			set tc [list $kw [lindex $cl 0] $at [lindex $cl 1]]
			if ![xincl str $tc] {
				lappend chd $tc
			}
		} elseif { $md == 2 } {
			# opening, self-closing
			set tc [list $kw "" $at ""]
			if ![xincl str $tc] {
				lappend chd $tc
			}
		} else {
			# closing
			if { $kw != $kwd } {
				error "mismatched tag: <$kwd ...> </$kw>"
			}
			# we are done with the tag - check for file
			# inclusion
			return [list $txt $chd]
		}
	}
}

proc xincl { s tag } {
#
# Process an include tag
#
	set kw [lindex $tag 0]

	if { $kw != "include" && $kw != "xi:include" } {
		return 0
	}

	set fn [sxml_attr $tag "href"]

	if { $fn == "" } {
		error "href attribute of <$kw ...> is empty"
	}

	if [catch { open $fn "r" } fd] {
		error "cannot open include file $fn: $fd"
	}

	if [catch { read $fd } fi] {
		catch { close $fd }
		error "cannot read include file $fn: $fi"
	}

	# merge it
	upvar $s str

	set str $fi$str

	return 1
}

proc sxml_parse { s } {
#
# Builds the XML tree from the provided string
#
	upvar $s str

	set v [xadv str ""]

	return [list root [lindex $v 0] "" [lindex $v 1]]
}

proc sxml_name { s } {

	return [lindex $s 0]
}

proc sxml_txt { s } {

	return [string trim [lindex $s 1]]
}

proc sxml_attr { s n { e "" } } {

	if { $e != "" } {
		# flag to tell the difference between an empty attribute and
		# its complete lack
		upvar $e ef
	}

	set al [lindex $s 2]
	set n [string tolower $n]
	foreach a $al {
		if { [lindex $a 0] == $n } {
			if { $e != "" } {
				set ef 1
			}
			return [lindex $a 1]
		}
	}
	if { $e != "" } {
		set ef 0
	}
	return ""
}

proc sxml_children { s { n "" } } {

	set cl [lindex $s 3]

	if { $n == "" } {
		return $cl
	}

	set res ""

	foreach c $cl {
		if { [lindex $c 0] == $n } {
			lappend res $c
		}
	}

	return $res
}

proc sxml_child { s n } {

	set cl [lindex $s 3]

	foreach c $cl {
		if { [lindex $c 0] == $n } {
			return $c
		}
	}

	return ""
}

proc sxml_yes { item attr } {
#
# A useful shortcut
#
	if { [string tolower [string index [sxml_attr $item $attr] 0]] == \
		"y" } {
			return 1
	}
	return 0
}

namespace export sxml_*

### end of XML namespace ######################################################

}

namespace import ::XML::*

###############################################################################
# End of Mini XML parser ######################################################
###############################################################################


package provide log 1.0

namespace eval LOGGING {

variable Log

proc abt { m } {

	puts stderr $m
	exit 99
}

proc log_open { { fname "" } { maxsize "" } { maxvers "" } } {

	variable Log

	if { $fname == "" } {
		if ![info exists Log(FN)] {
			set Log(FN) "log"
		}
	} else {
		set Log(FN) $fname
	}

	if { $maxsize == "" } {
		if ![info exists Log(MS)] {
			set Log(MS) 5000000
		}
	} else {
		set Log(MS) $maxsize
	}

	if { $maxvers == "" } {
		if ![info exists Log(MV)] {
			set Log(MV) 4
		}
	} else {
		set Log(MV) $maxvers
	}

	if [info exists Log(FD)] {
		# close previous log
		catch { close $Log(FD) }
		unset Log(FD)
	}

	if [catch { file size $Log(FN) } fs] {
		# not present
		if [catch { open $Log(FN) "w" } fd] {
			abt "Cannot open log file $Log(FN): $fd"
		}
		# empty log
		set Log(SZ) 0
	} else {
		# log file exists
		if [catch { open $Log(FN) "a" } fd] {
			abt "Cannot open log file $log(FN): $fd"
		}
		set Log(SZ) $fs
	}
	set Log(FD) $fd
	set Log(CD) 0
}

proc rotate { } {

	variable Log

	catch { close $Log(FD) }
	unset Log(FD)

	for { set i $Log(MV) } { $i > 0 } { incr i -1 } {
		set tfn "$Log(FN).$i"
		set ofn $Log(FN)
		if { $i > 1 } {
			append ofn ".[expr $i - 1]"
		}
		catch { file rename -force $ofn $tfn }
	}

	log_open
}

proc outlm { m } {

	variable Log

	catch {
		puts $Log(FD) $m
		flush $Log(FD)
	}

	incr Log(SZ) [string length $m]
	incr Log(SZ)

	if { $Log(SZ) >= $Log(MS) } {
		rotate
	}
}

proc log { m } {

	variable Log 

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
			outlm "$hdr #### $today ####"
		} else {
			outlm "00:00:00 #### BIM! BOM! $today ####"
		}
		set Log(CD) $day
	}

	outlm "$hdr $m"
}

namespace export log*

### end of LOGGING namespace ##################################################

}

namespace import ::LOGGING::log*

###############################################################################
# End of Log functions
###############################################################################

package require xml 1.0
package require log 1.0

if [catch { package require mysqltcl } ] {
	set SQL_present 0
} else {
	set SQL_present 1
}

# how to locate the relevant values in OSSI input #############################

set VL(AVRP)	{
			{ "agg" d } { "slot" d } { "" s } { "" s } { "col" d }
			{ "slot" d } { "" s } { "" s }
			{ "" a }
		}

set VL(CSTS)	{
			{ ":" d } { "via" d } { "maj_freq" d }
			{ "min_freq" d } { "rx_span" d } { "pl" x }
		}
## c_fl removed for now, for compatibility with the old version
#			{ ":" d } { "via" d } { "maj_freq" d }
#			{ "min_freq" d } { "rx_span" d } { "pl" x }
#			{ "c_fl" x }

set VL(ASTS)	{
			{ ":" d } { "audit" d } { "plev" x }
		}
## a_fl removed for now, for compatibility with the old version
#			{ ":" d } { "freq" d } { "plev" x } { "a_fl" x }

###############################################################################
###############################################################################

#
# Database storage function (by Andrew Hoyer)
#
proc data_out_db { cid lab val } {

	global DBFlag Time DBASE

	if { $DBFlag != "Y" } {
		# no external database
                msg "no external database"
		return
	}

	if ![info exists DBASE(name)] {
		msg "database undefined, -x request ignored"
		return
	}


        #msg "DBINFO: -user $DBASE(user) -db $DBASE(name) -host\
        #        $DBASE(host) -password $DBASE(password) "

	if [catch { mysql::connect -user $DBASE(user) -db $DBASE(name) -host\
		$DBASE(host) -password $DBASE(password) } con] {

		msg "SQL connection failed: $con"
		return
	}

	# transform sensor label to sid
	if [catch { set sid $DBASE(=$lab) } ] {
		# ignore if not mapped
		msg "sensor $lab not mapped in the database, ignored"
		catch { mysql::close $con }
		return
	}

	set timestamp [clock format $Time -format %Y%m%d%H%M%S]



	if [catch { mysql::exec $con "INSERT INTO observations\
	  (NID, SID, time, value) VALUES ($cid, $sid, $timestamp, $val)" } nr] {
		msg "SQL query failed: $nr"
	}

	catch { mysql::close $con }
}

#
# Data storage function
#
proc data_out { cid lab nam val } {
#
# Write the sensor value to the data log
#
	global Files Time

	if { $Files(DATA) == "" } {
		# no data logging
                #msg "no data logging."
		return
	}

	set day [clock format $Time -format %y%m%d]

	if { $day != $Files(DATA,DY) } {
		# close the current file
		catch { close $Files(DATA,FD) }
		set Files(DATA,DY) $day
		set fn $Files(DATA)_$day
		if [catch { open $fn "a" } fd] {
			msg "cannot open data log file $fn: $fd"
			set Files(DATA) ""
			return
		}

		set Files(DATA,FD) $fd
	}

	catch {
		puts $Files(DATA,FD) \
		  "[clock format $Time -format %H%M%S] $cid +$lab+ @$nam@ =$val"
		flush $Files(DATA,FD)
	}
}

###############################################################################

#
# Error detectors and handlers
#

proc nannin { n } {
#
# Not A NonNegative Integer Number
#
	upvar $n num

	if [catch { expr $num } num] {
		return 1
	}
	set t $num
	if [catch { incr t }] {
		return 1
	}
	if { $num < 0 } {
		return 1
	}
	return 0
}

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

proc nan { n } {
#
# Not a number
#
	if [catch { expr $n }] {
		return 1
	}
	return 0
}

proc badsnip { c } {
#
# Bad converter snippet
#
	set value 10
	if [catch { eval $c } err] {
		return 1
	}

	return [nan $value]
}

proc msg { m } {

	puts $m
	log $m
}

proc abt { m } {

	msg "aborted: $m"
	exit 99
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
		msg "connecting to a VUEE model: node $Uart(NODE),\
			host $Uart(HOST), port $Uart(PORT) ..."

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

#
# Sensor functions
#

proc smerr { m } {
#
# Handles errors in sensor map file
#
	global Converters

	if ![info exists Converters] {
		# first time around, have to abort
		abt $m
	}

	msg $m
	msg "continuing with old sensor map settings"
}

proc read_map { } {
#
# Read the map file, returns 1 on success (i.e., a new map file has been read)
# and 0 on failure; the first time around, it isn't allowed to fail (see smerr)
#
	global Files Time

	global SBN		;# sensors by node (an array)
	global Collectors	;# list of collector nodes (IDs)
	global Aggregators	;# list of aggregator nodes (IDs)
	global Sensors		;# list of all sensors in declaration order
	global Converters	;# list of converter snippets
	global Nodes		;# the array of all nodes
	global OSSI		;# OSSI aggregator node (ID)
	global DBASE		;# database parameters
	global OPERATOR

	if [catch { file mtime $Files(SMAP) } ix] {
		smerr "cannot stat map file $Files(SMAP): $ix"
		return 0
	}
	# check the time stamp of the map file
	if { [info exists Files(SMAP,TS)] && $Files(SMAP,TS) >= $ix } {
		# not changed, do nothing
		return 0
	}
	# update the time stamp; even if we fail, we will not try the same file
	# again, until it is replaced
	set Files(SMAP,TS) $ix

	if [catch { open $Files(SMAP) r } fd] {
		smerr "cannot open map file $Files(SMAP): $fd"
		return 0
	}

	if [catch { read $fd } sm] {
		smerr "cannot read map file $Files(SMAP): $sm"
		return 0
	}

	catch { close $fd }

	msg "read map file: $Files(SMAP)"

	if [catch { sxml_parse sm } sm] {
		smerr "map file error, $sm"
		return 0
	}

	set sm [sxml_child $sm "map"]
	if { $sm == "" } {
		smerr "map tag not found in the map file"
		return 0
	}

	# initialize a few items; the globals are only changed if the whole
	# update turns out to be formally correct
	set colls ""		;# collectors
	set aggts ""		;# aggregators
	set snsrs ""		;# sensors
	set cnvts ""		;# converters

	# collect node info: collectors #######################################

	set cv [sxml_children [sxml_child $sm "collectors"] "node"]
	if { $cv == "" } {
		smerr "no collectors in map file"
		return 0
	}

	set ix 0
	foreach c $cv {
		incr ix
		set sn [sxml_attr $c "id"]
		if [napin sn] {
			smerr "illegal or missing id in collector $ix"
			return 0
		}
		if [info exists nodes($sn)] {
			smerr "duplicate node id in collector $ix"
			return 0
		}
		# legit so far
		set pl [sxml_attr $c "power"]
		if { $pl == "" } {
			# the default
			set pl 7
		} else {
			if [nannin pl] {
				smerr "illegal power level in collector $ix"
				return 0
			}
		}
		set fr [sxml_attr $c "frequency"]
		if { $fr != "" } {
			if [napin fr] {
				smerr "illegal frequency in collector $ix"
				return 0
			}
		} else {
			# this gives us something to send if we don't know the
			# aggregator and also tells us that we should inherit
			# the aggregator's report frequency
			set fr -1
		}
		set xs [sxml_attr $c "rxspan"]
		if { $xs != "" } {
			if [nannin xs] {
				smerr "illegal rx span in collector $ix"
				return 0
			}
			if { $xs > 30 } {
				set xs 1
			} else {
				# in seconds
				set xs [expr $xs * 1024]
			}
		} else {
			set xs -1
		}

		#
		# the record layout is the same for both node types:
		# 	- class (c,a)
		#	- text (can be used as a description)
		#	- configuration parameters (a list)
		#	- dynamic parameters (another list)
		#
		set nodes($sn) [list c [sxml_txt $c] [list $pl $fr $xs] ""]
		# this is just a list of IDs
		lappend colls $sn
	}

	# aggregators #########################################################

	set cv [sxml_children [sxml_child $sm "aggregators"] "node"]
	if { $cv == "" } {
		smerr "no aggregators in map file"
		return 0
	}

	set ix 0
	foreach c $cv {
		incr ix
		set sn [sxml_attr $c "id"]
		if [napin sn] {
			smerr "illegal or missing id in aggregator $ix"
			return 0
		}
		if [info exists nodes($sn)] {
			smerr "duplicate node id in aggregator $ix"
			return 0
		}
		set pl [sxml_attr $c "power"]
		if { $pl == "" } {
			set pl 7
		} else {
			if [nannin pl] {
				smerr "illegal power level in aggregator $ix"
				return 0
			}
		}
		set fr [sxml_attr $c "frequency"]
		if { $fr == "" } {
			# some default
			set fr 60
		} else {
			if [napin fr] {
				smerr "illegal frequency in aggregator $ix"
				return 0
			}
		}
		set nodes($sn) [list a [sxml_txt $c] [list $pl $fr] ""]
		lappend aggts $sn
	}

	# the OSSI aggregator #################################################

	set cv [sxml_child $sm "ossi"]
	if { $cv == "" } {
		smerr "no ossi node"
		return 0
	}
	set ossi [sxml_attr $cv "id"]
	if [napin ossi] {
		smerr "aggregator id in ossi spec missing or illegal"
		return 0
	}

	if { ![info exist nodes($ossi)] || [lindex $nodes($ossi) 0] != "a" } {
		smerr "ossi node is not an aggregator"
		return 0
	}

	# the converter snippets ##############################################

	set cv [sxml_children [sxml_child $sm "converters"] "snippet"]

	set cnvts ""

	set ix 0
	foreach c $cv {
		incr ix
		set sn [sxml_attr $c "sensors"]
		if { $sn == "" } {
			smerr "snippet number $ix has no sensors attribute"
			return 0
		}
		set co [sxml_txt $c]
		if { $co == "" } {
			smerr "snippet number $ix has empty code"
			return 0
		}
		if [badsnip $co] {
			smerr "snippet number $ix doesn't execute"
			return 0
		}
		lappend cnvts [list $sn $co]
	}

	# the sensors #########################################################

	set cv [sxml_children [sxml_child $sm "sensors"] "sensor"]
	set ix 0
	foreach c $cv {

		# sensor number starting from zero
		set si $ix
		# these ones are from 1 for error messages
		incr ix

		set sn [sxml_attr $c "name"]
		set la [sxml_attr $c "label"]
		set no [sxml_attr $c "node"]
		set de [sxml_txt $c]

		if { $sn == "" } {
			smerr "sensor $ix has no name"
			return 0
		}
		if { $la == "" } {
			smerr "sensor $ix has no label"
			return 0
		}
		if [napin no] {
			smerr "missing or illegal node id in sensor $ix"
			return 0
		}
		if { ![info exists nodes($no)] || \
		    [lindex $nodes($no) 0] != "c" } {
			smerr "node id ($no) for sensor $ix is not a collector"
			return 0
		}
		# get hold of the sensor list for node $no
		if [info exists sbn($no)] {
			set sl $sbn($no)
		} else {
			set sl ""
		}
		# check if a sensor with the name is not present already
		if [info exist snames($sn)] {
			smerr "sensor $sn multiply defined"
			return 0
		}
		set snames($sn) ""
		# just the index
		lappend sl $si
		# this is where we stroe full sensor record
		set sbn($no) $sl

		# add to the global list; the index is equal to the sensor's
		# position on this list
		lappend snsrs [list $sn $la $no $de]
	}

	# db parameters #######################################################

	set cv [sxml_child $sm "database"]
	if { $cv != "" } {

		# database parameters are present

		set dn [sxml_attr $cv "name"]
		if { $dn == "" } {
			smerr "name attribute missing for database"
			return 0
		}
		set dbase(name) $dn
		set dbase(user) [sxml_txt [sxml_child $cv "user"]]
		set dbase(password) [sxml_txt [sxml_child $cv "password"]]
		set dbase(host) [sxml_txt [sxml_child $cv "host"]]

		set cv [sxml_txt [sxml_child $cv "sensormap"]]
		while { [regexp \
		  "(\[^ \t\n\]+)\[^ \t\n\]*=\[^ \t\n\]*(\[^ \t\n\]+)" $cv m \
		    nm nu] } {
			set nm [string trim $nm "\""]
			set nu [string trim $nu "\""]
			if { $nm == "" || $nu == "" } {
				smerr "illegal entry in dbmap: $m"
				return 0
			}
			if ![regexp "^\[0-9\]+$" $nu] {
				smerr "db index in dbmap: $nu is not numeric"
				return 0
			}
			if [info exists dbase(=$nm)] {
				smerr "duplicate name in database sensor map:\
					$nm"
				return 0
			}
			set dbase(=$nm) $nu
			# remove the match from the string
			set nm [string first $m $cv]
			set cv [string range $cv [expr $nm + [string length \
				$m]] end]
		}
	}

	# the operator (more to come later) ###################################

	set em ""
	foreach cv [sxml_children [sxml_child $sm "operator"] "email"] {
		lappend em [sxml_txt $cv]
	}
			
	# hot swap the tables #################################################

	set Converters $cnvts
	set Sensors $snsrs
	set Collectors $colls
	set Aggregators $aggts
	set OSSI $ossi

	array unset SBN
	array unset Nodes
	array unset DBASE
	array unset OPERATOR

	foreach no [array names nodes] {
		set Nodes($no) $nodes($no)
		# Initialize report times/intervals
		set Nodes($no,RT) $Time
		set Nodes($no,RI) 0
	}

	foreach no [array names sbn] {
		set SBN($no) $sbn($no)
	}

	foreach no [array names dbase] {
		set DBASE($no) $dbase($no)
	}

	set OPERATOR(email) $em

	set oms "[llength $Aggregators] aggregators,\
		[llength $Collectors] collectors, [llength $Sensors] sensors,\
			[llength $Converters] snippets"
	msg $oms

	operator_alert "OSSI daemon reset: $oms"

	return 1
}

proc encode_sv { v t } {
#
# Encodes a sensor value into a 32-byte block
#
	return [format "%16.8g  %14d" $v $t]
}

proc value_update { s v } {
#
# Writes a sensor value to the file
#
	global Files Time

	if { $Files(SVAL) == "" } {
		# no values file
		return
	}

	seek $Files(SVAL,FD) [expr $s << 5]
	puts -nonewline $Files(SVAL,FD) [encode_sv $v $Time]
	flush $Files(SVAL,FD)
}

proc values_init { } {
#
# Initialize the values file
#
	global Files Time Sensors

	if { $Files(SVAL) == "" } {
		# no values file
		return
	}

	if [info exists Files(SVAL,FD)] {
		catch { close $Files(SVAL,FD) }
	}

	# open or re-open
	if [catch { open $Files(SVAL) "w" } fd] {
		abt "cannot open output file $Files(SVAL): $fd"
	}

	set Files(SVAL,FD) $fd

	# the number of slots
	set ns [llength $Sensors]

	for { set i 0 } { $i < $ns } { incr i } {
		value_update $i 0.0
	}

}

###############################################################################

proc iran { val } {
#
# Randomize the specified integer value within 30%
#
	return [expr int($val * (1.0 + 0.3 * (rand() * 2.0 - 1.0)))]
}

proc lrep { lst ix el } {
#
# Replaces element ix in the list ls (my version)
#
	set k [llength $lst]
	if { $k <= $ix } {
		while { $k < $ix } {
			lappend lst ""
			incr k
		}
		lappend lst $el
		return $lst
	}

	return [lreplace $lst $ix $ix $el]
}

proc exnum { str pat } {
#
# Extracts values from the string according to the specified pattern
#
	global VL

	set res ""

	foreach p $VL($pat) {

		set kwd [lindex $p 0]
		set typ [lindex $p 1]

		if { $kwd != "" } {
			if ![regexp -nocase -indices $kwd $str ind] {
				return ""
			}

			set str [string trimleft \
				[string range \
					$str [expr [lindex $ind 1] + 1] end]]
		} else {
			set str [string trimleft $str]
		}

		if { $typ == "a" } {
			# everything
			lappend res [string trimright $str]
			return $res
		}

		if ![regexp "^\[^ \t\]+" $str num] {
			return ""
		}

		# remove the present component for further matching
		set str [string range $str [string length $num] end]

		if { $typ == "s" } {
			# string
			lappend res $num
			continue
		}

		if { $typ == "x" || $typ == "h" } {
			regexp -nocase "\[0-9a-f\]+" $num num
			set num "0x$num"
		} else {
			regexp -nocase "\[0-9\]+" $num num
		}
		if [catch { expr $num } num] {
			return ""
		}

		lappend res $num
	}
	return $res
}

proc get_assoc_agg { co } {
#
# Get the aggregator of the specified collector
#
	global Nodes

	if ![info exists Nodes($co)] {
		# quietly ignore, this cannot happen
		return ""
	}

	return [lindex [lindex $Nodes($co) 3] 3]
}
		
proc assoc_agg { ag co } {
#
# Associates the collector with the aggregator (or the other way around, if you
# prefer)
#
	global Nodes

	if ![info exists Nodes($co)] {
		# quietly ignore
		return
	}

	set col $Nodes($co)

	# dynamic parameters
	set dp [lindex $col 3]

	set ca [lindex $dp 3]

	if { $ca != $ag } {
		if { $ag == "" } {
			msg "collector $co detached from aggregator $ca"
		} elseif { $ca != "" } {
			msg "collector $co changed aggregator $ca -> $ag"
		} else {
			msg "collector $co attached to aggregator $ag"
		}
		set Nodes($co) [lrep $col 3 [lrep $dp 3 $ag]]
	}
}

proc get_rep_freq { no } {
#
# Return the target reporting frequency for the indicated node (agg or col)
#
	global Nodes

	if ![info exists Nodes($no)] {
		# cannot happen
		return -1
	}

	set node $Nodes($no)
	set fr [lindex [lindex $node 2] 1]
	if { $fr == "" } {
		# cannot happen
		set fr -1
	}
	if { [lindex $node 0] == "a" } {
		return $fr
	}

	# the collector case
	if { $fr != -1 } {
		# specifies own frequency
		return $fr
	}

	# look at the associated aggregator
	set no [get_assoc_agg $no]
	if { $no == "" } {
		# no way
		return -1
	}

	if ![info exists Nodes($no)] {
		return -1
	}

	set node $Nodes($no)
	set fr [lindex [lindex $node 2] 1]
	if { $fr == "" } {
		set fr -1
	}

	return $fr
}

proc update_report_interval { no } {
#
# Updates the report time, which is used to detect dead nodes as well as their
# compliance with the declared report frequency
#
	global Nodes Time

	if [catch { expr $Time - $Nodes($no,RT) } delta] {
		set delta 0
	}
	set Nodes($no,RI) $delta
	set Nodes($no,RT) $Time
	if [info exists Nodes($no,AP)] {
		# clear pending alert flag
		unset Nodes($no,AP)
		operator_alert "Node $no is back"
	}
}

proc input_line { inp } {
#
# Handle line input from UART
#
	global Time

	set inp [string trim $inp]

	# write the line to the log (and standard output)
	msg "<- $inp"

	if ![regexp "^(\[0-9\]\[0-9\]\[0-9\]\[0-9\]) (.*)" $inp jnk tp inp] {
		# every line of significance to us must begin with a four
		# digit identifier
		return
	}

	set Time [clock seconds]

	switch $tp {

	1002 { input_avrp $inp }
	1005 { input_asts $inp }
	1006 { input_csts $inp }
	0003 { input_mast $inp }
	8000 { input_eful $inp }

	}
}

proc input_eful { inp } {
#
# EEPROM full on the master
#
	global Nodes OSSI

	if { [string first "EEPROM FULL" $inp] < 0 } {
		return
	}

	set ag $Nodes($OSSI)
	set dp [lindex $ag 3]
	set osc [lindex $dp 3]

	if { $osc == "" || $osc >= 4 } {
		# either haven't tried to erase yet, or have waited for 4
		# cycles and nothing happened
		msg "queueing eeprom erase for aggregator $OSSI (master)"
		# once
		roster_schedule "cmd_aerase $OSSI"
		set osc 0
	}

	incr osc

	set dp [lrep $dp 3 $osc]
	set Node($OSSI) [lrep $ag 3 $dp]
}

proc input_mast { inp } {
#
# Master beacon notification
#
	msg "master beacon acknowledge"
	roster_schedule "cmd_master" [iran 3600] 3600
}

proc input_asts { inp } {
#
# Aggregator statistics
#
	global Nodes

	set res [exnum $inp ASTS]
	if { $res == "" } {
		msg "erroneous aggregator statistics line: $inp"
		return
	}

	set aid [lindex $res 0]

	# locate this aggregator
	if { [napin aid] || ![info exists Nodes($aid)] } {
		msg "statistics for unknown aggregator $aid, ignored"
		return
	}

	set ag $Nodes($aid)
	if { [lindex $ag 0] != "a" } {
		# this isn't an aggregator
		msg "collector $aid reporting stats as an aggregator"
		return
	}

	# for now we shall only extract 2 values: frequency and power level

	set fr [lindex $res 1]
	set pl [lindex $res 2]

	# expected frequency and power
	set sp [lindex $ag 2]
	set tp [lindex $sp 0]
	set tf [lindex $sp 1]

	if { $tp == $pl && $tf == $fr } {
		# ten minutes until next poll
		msg "aggregator params acknowledge: $aid = ($fr, $pl)"
		set rate 1800
	} else {
		# until next poll
		msg "aggregator params still wrong: $aid = ($fr, $pl)\
			!= ($tf, $tp)"
		set rate 60
	}

	roster_schedule "cmd_apoll $aid" [iran $rate] $rate
}

proc input_csts { inp } {
#
# Collector statistics
#
	global Nodes

	set res [exnum $inp CSTS]
	if { $res == "" } {
		msg "erroneous collector statistics line: $inp"
		return
	}

	set cid [lindex $res 0]

	# locate this collector
	if { [napin cid] || ![info exists Nodes($cid)] } {
		msg "statistics from unknown collector $cid, ignored"
		return
	}

	set co $Nodes($cid)
	if { [lindex $co 0] != "c" } {
		# this isn't a collector
		msg "aggregator $cid sending stats as a collector"
		return
	}

	# FIXME: should detect when memory is filled and do something;
	# also concerns the aggregator

	set fr [lindex $res 2]
	set pl [lindex $res 5]

	# expected frequency and power
	set sp [lindex $co 2]
	set tp [lindex $sp 0]
	set tf [lindex $sp 1]

	if { $tp == $pl && $tf == $fr } {
		# ten minutes until next poll
		msg "collector params acknowledge: $cid = ($fr, $pl)"
		set rate 3600
	} else {
		# until next poll
		msg "collector params still wrong: $cid = ($fr, $pl)\
			!= ($tf, $tp)"
		set rate 240
	}

	roster_schedule "cmd_cpoll $cid" [iran $rate] $rate
}

proc input_avrp { inp } {
#
# Aggregator/sensor value report
#
	global Nodes Time SBN Sensors Converters OSSI

	set res [exnum $inp AVRP]

	if { $res == "" } {
		msg "erroneous agg report line: $inp"
		return
	}

	set aid [lindex $res 0]

	# locate this aggregator
	if { [napin aid] || ![info exists Nodes($aid)] } {
		msg "report from unknown aggregator $aid, ignored"
		return
	}

	set ag $Nodes($aid)
	if { [lindex $ag 0] != "a" } {
		# this isn't an aggregator
		msg "collector $aid reporting as an aggregator"
		return
	}

	# update the aggregator's report interval
	update_report_interval $aid

	set asl [lindex $res 1]
	set ats "[lindex $res 2] [lindex $res 3]"
	set cid [lindex $res 4]

	# dynamic parameters; for now, we only store the slot number
	set dp [lindex $ag 3]
	set osc [lindex $dp 3]
	set dp [list $Time $asl $ats $osc]
	set Nodes($aid) [lrep $ag 3 $dp]

	# locate the collector
	if { [napin cid] || ![info exists Nodes($cid)] } {
		msg "report from unknown collector $cid, ignored"
		return
	}

	set co $Nodes($cid)
	if { [lindex $co 0] != "c" } {
		# this isn't a collector
		msg "aggregator $cid reporting as a collector"
		return
	}

	if { [string first "gone" $inp] >= 0 } {
		# this is void, so ignore it
		msg "collector $cid is gone, values ignored"
		assoc_agg "" $cid
		roster_schedule "cmd_cpoll $cid" [iran 10] 240
		return
	}

        # update collector report interval
        update_report_interval $cid

	set csl [lindex $res 5]
	set cts "[lindex $res 6] [lindex $res 7]"

	# dynamic parameters
	set dp [lindex $co 3]
	# previous aggeregator
	set oai [lindex $dp 3]
	# last slot
	set osl [lindex $dp 1]

	if { $osl != "" && $osl == $csl } {
		# EEPROM full on the collector
		msg "eeprom status unchanged on collector $cid"
	}
	set dp [list $Time $csl $cts $oai]
	set Nodes($cid) [lrep $co 3 $dp]

	# associate the aggregator
	assoc_agg $aid $cid

	# extract sensor values
	set inp [lindex $res 8]
	set uc 0
	while 1 {

		set inp [string trimleft $inp]
		if ![regexp "\[0-9\]+" $inp value] {
			# all done
			break
		}

                #msg "Value 1 = $value"
		# remove from the string
		set ix [expr [string first $value $inp] + \
			[string length $value]]
		set inp [string range $inp $ix end]

                #msg "Value 2 = $value"
		# locate the sensor
		set nf 1
		foreach s $SBN($cid) {
			set se [lindex $Sensors $s]
			if { [lindex $se 1] == $uc } {
				# found
				set nf 0
				break
			}
		}

		if $nf {
			continue
		}

		# locate the conversion snippet
		set sn [lindex $se 0]
		set nf 1
		foreach cv $Converters {
			if [regexp [lindex $cv 0] $sn] {
				set nf 0
				break
			}
		}

                #msg "Value before convert  = $value"
		if { !$nf } {
			# convert
			set co [lindex $cv 1]
                #        msg "co = $co"
                #        msg "eval co = {eval $co}"
			if [catch { eval $co } err] {
				msg "conversion failed for $sn, collector $cid:\
					$err"
				continue
			} 

		}

                #msg "Value after convert  = $value"

		# the value is ready
		value_update $s $value
		# database
                #msg "data_out_db $cid $uc  $value"
		data_out_db $cid $uc $value
		# log
		data_out $cid $uc $sn $value

		incr uc
	}

	msg "updated $uc values from collector $cid"
}

###############################################################################

proc cmd_cpoll { co } {
#
# Polls a collector
#
	global Nodes

	set ag [get_assoc_agg $co] 
	if { $ag == "" } {
		# we don't know the aggregator, ask them
		uart_write "f $co"
		return
	}

	# we know the aggregator

	set no $Nodes($co)
	# static parameters
	set dp [lindex $no 2]

	uart_write "c $co $ag [lindex $dp 1] -1 [lindex $dp 2] [lindex $dp 0]"
}

proc cmd_apoll { ag } {
#
# Polls an aggregator
#
	global Nodes

	# configuration parameters
	set no [lindex $Nodes($ag) 2]

	uart_write "a $ag [lindex $no 1] [lindex $no 0]"
}

proc cmd_master { } {
#
# Request our node to become master
#
	uart_write "m"
}

proc cmd_aerase { ag } {
#
# Request to erase EEPROM at the aggregator
#
	global OSSI

	if { $ag == $OSSI } {
		uart_write "E"
	}
}

proc cmd_cerase { ag co } {
#
# Can this be done at all?
#
	return
}

proc external_command { } {
#
# Handle an external command
#
	global Files

	if ![file exists $Files(ECMD)] {
		return
	}

	if [catch { open $Files(ECMD) r } fd] {
		abt "cannot open external command file $Files(ECMD): $fd"
	}

	if [catch { read $fd } cmd] {
		abt "cannot read external command file $Files(ECMD): $cmd"
	}

	catch { close $fd }

	# check if the line is complete
	set nl [string first "\n" $cmd]
	if { $nl < 0 } {
		return
	}

	set cmd [string trim [string range $cmd 0 $nl]]

	if { $cmd != "" } {
		# issue it
		msg "external command: $cmd"
		roster_schedule "uart_write \"$cmd\"" "" ""
	}

	# delete the file
	catch { file delete -force $Files(ECMD) }
}

proc report_interval_monitor { } {
#
# Monitors reporting intervals and alerts the operator to dead nodes
#
	global Nodes Aggregators Collectors Time

	foreach no $Aggregators {
		if [info exists Nodes($no,AP)] {
			# alert already issued
			continue
		}
		if [catch { expr $Time - $Nodes($no,RT) } delta] {
			# impossible
			set Nodes($no,RT) $Time
			continue
		}
		if { $delta > 1800 } {
			# 30 minutes
			set Nodes($no,AP) ""
			operator_alert \
				"Aggregator $no hasn't reported for 30 minutes"
		}
	}
	
	foreach no $Collectors {
		if [info exists Nodes($no,AP)] {
			# alert already issued
			continue
		}
		if [catch { expr $Time - $Nodes($no,RT) } delta] {
			# impossible
			set Nodes($no,RT) $Time
			continue
		}
		if { $delta > 3600 } {
			# one hour
			set Nodes($no,AP) ""
			operator_alert \
				"Collector $no hasn't reported for one hour"
		}
	}
}

proc operator_alert { msg } {
#
# Sends an alert to the operator
#
	global OPERATOR

	msg "Operator alert: $msg"
	append msg "\n"

	if [info exists OPERATOR(email)] {
		foreach em $OPERATOR(email) {
			catch {
				msg "sending email to $em"
				exec mail -s "EcoNet Alert" $em << "$msg"
			}
		}
	}
}
			
###############################################################################

#
# Roster operations
#
# The roster is a list of action items, each item looking as follows:
#
#    - Time		(when to run)
#    - function		(what to call)
#    - intvl		(interval for periodic rescheduling)
#

proc roster_schedule { fun { tim "" } { del "" } } {
#
# Add a command to the roster
#
	global Roster Time

	if { $tim == "" } {
		set tim 0
	}
	incr tim $Time
		
	# check if the indicated fun already exists
	set ix 0
	set fo 0
	foreach ai $Roster {
		if { [lindex $ai 1] == $fun } {
			set fo 1
			break
		}
		incr ix
	}

	if $fo {
		# remove the old version
		set Roster [lreplace $Roster $ix $ix]
	}

	# the new action item
	set cai [list $tim $fun $del]

	# find the place to insert
	set ix 0
	foreach ai $Roster {
		if { [lindex $ai 0] > $tim } {
			break
		}
		incr ix
	}

	set Roster [linsert $Roster $ix $cai]
}

proc roster_run { } {
#
# Execute the roster
#
	global Roster Time

	set cai [lindex $Roster 0]
	if { $cai == "" } {
		return
	}

	if { [lindex $cai 0] > $Time } {
		return
	}

	# remove the first element
	set Roster [lrange $Roster 1 end]

	set fun [lindex $cai 1]
	set del [lindex $cai 2]

	eval $fun

	if { $del != "" } {
		roster_schedule $fun $del $del
	}
}

proc roster_init { } {
#
# Initialize the roster
#
	global Roster Aggregators Collectors Nodes Time

	#
	# Aggregator parameters starting from our OSS
	#

	# make sure the slate is clean; you may be doing this after a
	# reconfiguration, so do not assume anything
	set Roster ""

	# make sure our node is the master; repeat at 30 sec intervals until
	# confirmed
	roster_schedule "cmd_master" "" 30

	# set up the parameters of aggregators
	foreach ag $Aggregators {
		roster_schedule "cmd_apoll $ag" 2 30
	}

	set tm 10
	foreach co $Collectors {
		roster_schedule "cmd_cpoll $co" $tm 60
		incr tm 10
	}

	# report interval monitor
	roster_schedule "report_interval_monitor" 60 300
}

###############################################################################

proc loop { } {
#
# The main loop
#
	global Uart Time Turn

	set Turn 0

	while 1 {

		if { $Uart(FD) == "" } {
			# we are not connected
			if { $Uart(MODE) < 2 } {
				# that's it
				break
			}
			vwait Turn
			continue
		}

		set Time [clock seconds]

		if [read_map] {
			# a new map: (re) initialize things
			values_init
			roster_init
		}

		# check for external command
		external_command

		roster_run
		# delay for one second
		after 1000 { incr Turn }
		vwait Turn
	}

	msg "terminated"
}

#
# Process the arguments
#

proc bad_usage { } {

	global argv0

	puts "Usage: $argv0 options, where options can be:\n"
	puts "       -h hostname, default is localhost"
	puts "       -p port, default is 4443"
	puts "       -n OSSI_node_number (VUEE), default is 0"
	puts "       -u uart_device, default is VUEE (no -u)"
	puts "       -e uart_encoding_params, default is 9600,n,8,1"
	puts "       -d data_logging_file_prefix, default is no datalogging"
	puts "       -l log_file_name, default is log"
	puts "       -s sensor_map_file, default is sensors.xml"
	puts "       -v values_file, default is values"
	puts "       -x (send output to the DB described in sensors.xml)"
	puts "       -c external_command_file, default is command"
	puts ""
	exit 99
}

set Uart(MODE) ""
set DBFlag ""

while { $argv != "" } {

	set fg [lindex $argv 0]
	set argv [lrange $argv 1 end]
	set va [lindex $argv 0]
	if { $va == "" || [string index $va 0] == "-" } {
		set va ""
	} else {
		set argv [lrange $argv 1 end]
	}

	if { $fg == "-h" } {
		if { $Uart(MODE) == 0 } {
			bad_usage
		}
		if [info exists Uart(HOST)] {
			bad_usage
		}
		if { $va != "" } {
			set Uart(HOST) $va
		} else {
			set Uart(HOST) "localhost"
		}
		set Uart(MODE) 1
		continue
	}

	if { $fg == "-p" } {
		if { $Uart(MODE) == 0 } {
			bad_usage
		}
		if [info exists Uart(PORT)] {
			bad_usage
		}
		if { $va != "" } {
			if { [napin va] || $va > 65535 } {
				bad_usage
			}
			set Uart(PORT) $va
		} else {
			# the default
			set Uart(PORT) 4443
		}
		set Uart(MODE) 1
		continue
	}

	if { $fg == "-n" } {
		if { $Uart(MODE) == 0 } {
			bad_usage
		}
		if [info exists Uart(NODE)] {
			bad_usage
		}
		if { $va != "" } {
			if [napin va] {
				bad_usage
			}
			set Uart(NODE) $va
		} else {
			# the default
			set Uart(NODE) 0
		}
		set Uart(MODE) 1
		continue
	}

	if { $fg == "-u" } {
		if { $Uart(MODE) == 1 } {
			bad_usage
		}
		if [info exists Uart(DEV)] {
			bad_usage
		}
		if { $va != "" } {
			set Uart(DEV) $va
		} else {
			# must be specified
			bad_usage
		}
		set Uart(MODE) 0
		continue
	}

	if { $fg == "-e" } {
		if { $Uart(MODE) == 1 } {
			bad_usage
		}
		if [info exists Uart(PAR)] {
			bad_usage
		}
		if { $va != "" } {
			set Uart(PAR) $va
		} else {
			# default
			set Uart(PAR) "9600,n,8,1"
		}
		set Uart(MODE) 0
		continue
	}

	if { $fg == "-d" } {
		if [info exists Files(DATA)] {
			bad_usage
		}
		if { $va == "" } {
			# this is the prefix
			set Files(DATA) "data"
		} else {
			set Files(DATA) $va
		}
		continue
	}

	if { $fg == "-x" } {
		if { $DBFlag != "" } {
			bad_usage
		}
		if { !$SQL_present } {
			puts "No SQL available, -x option ignored"
			set DBFlag "N"
		} else {
			set DBFlag "Y"
		}
		continue
	}

	if { $va == "" } {
		bad_usage
	}

	if { $fg == "-l" } {
		if [info exists Files(LOG)] {
			bad_usage
		}
		set Files(LOG) $va
		continue
	}

	if { $fg == "-s" } {
		if [info exists Files(SMAP)] {
			bad_usage
		}
		set Files(SMAP) $va
		continue
	}

	if { $fg == "-v" } {
		if [info exists Files(SVAL)] {
			bad_usage
		}
		set Files(SVAL) $va
		continue
	}

	if { $fg == "-c" } {
		if [info exists Files(ECMD)] {
			bad_usage
		}
		set Files(ECMD) $va
		continue
	}
}

if { $Uart(MODE) == "" } {
	# all defaults
	set Uart(MODE) 1
	set Uart(HOST) "localhost"
	set Uart(PORT) 4443
	set Uart(NODE) 0
} elseif $Uart(MODE) {
	# VUEE
	if ![info exists Uart(HOST)] {
		set Uart(HOST) "localhost"
	}
	if { $Uart(HOST) == "server" } {
		set Uart(MODE) 2
		if ![info exists Uart(PORT)] {
			# the default is different
			set Uart(PORT) 4445
		}
	} elseif ![info exists Uart(PORT)] {
		set Uart(PORT) 4443
	}
	if ![info exists Uart(NODE)] {
		set Uart(NODE) 0
	}
} else {
	if ![info exists Uart(DEV)] {
		bad_usage
	}
	if ![info exists Uart(PAR)] {
		set Uart(PAR) "9600,n,8,1"
	}
}

if ![info exists Files(LOG)] {
	set Files(LOG) "log"
}

if ![info exists Files(SMAP)] {
	set Files(SMAP) "sensors.xml"
}

if ![info exists Files(SVAL)] {
	set Files(SVAL) ""
} elseif { $Files(SVAL) == "" } {
	set FIles(SVAL) "values"
}

if ![info exists Files(ECMD)] {
	set Files(ECMD) "command"
}

if ![info exists Files(DATA)] {
	set Files(DATA) ""
}

set Files(DATA,DY) ""

###############################################################################

set Turn 0
set Uart(FD) ""
	
log_open $Files(LOG)

uart_init input_line

loop
