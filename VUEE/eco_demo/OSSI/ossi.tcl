#!/bin/sh
###########################\
exec tclsh "$0" "$@"

##################################################################
# A simple OSS module for ECO DEMO                               #
#                                                                #
# Copyright (C) Olsonet Communications, 2008 All Rights Reserved #
##################################################################

set	Log(MAXSIZE)		5000000
set	Log(MAXVERS)		4

###############################################################################

#
# Mini XML parser
#

proc sxml_string { s } {
#
# Extract a possibly quoted string
#
	upvar $s str

	if { [sxml_space str] != "" } {
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
		return [sxml_unescape $val]
	}

	# the tricky way
	if ![regexp "^.(\[^\"\]*)\"" $str match val] {
		error "missing \" in string"
	}
	set str [string range $str [string length $match] end]

	return [sxml_unescape $val]
}

proc sxml_unescape { str } {
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

proc sxml_space { s } {
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

proc sxml_ftag { s } {
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
			# skip it
			set ix [string first "-->" $str]
			if { $ix < 0 } {
				error "unterminated comment: [string range \
					$str 0 15]"
			}
			incr ix 3
			set str [string range $str $ix end]
			continue
		}
		set et ""
		if [regexp -nocase "^<(/)?\[a-z_\]" $str ix et] {
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

	if ![regexp -nocase "^(\[a-z0-9_\]+)(.*)" $str ix kwd str] {
		# error
		error "illegal tag: [string range $str 0 15]"
	}

	set kwd [string tolower $kwd]

	# decode any attributes
	set attr ""
	array unset atts

	while 1 {
		sxml_space str
		if { $str == "" } {
			error "unterminated tag: <$et$kwd"
		}
		set c [string index $str 0]
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
		if [catch { sxml_string str } val] {
			error "illegal attribute value: \
				<$et$kwd ... $atr=[string range $str 0 15]"
		}
		lappend attr [list $atr $val]
	}
}

proc sxml_advance { s kwd } {
#
# Returns the text + the list of children for the current tag
#
	upvar $s str

	set txt ""
	set chd ""

	while 1 {
		# locate the nearest tag
		set tag [sxml_ftag str]
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
			# opening
			set cl [sxml_advance str $kw]
			set tc [list $kw [lindex $cl 0] $at [lindex $cl 1]]
			lappend chd $tc
		} else {
			# closing (must be ours)
			if { $kw != $kwd } {
				error "mismatched tag: <$kwd ...> </$kw>"
			}
			# we are done
			return [list $txt $chd]
		}
	}
}

proc sxml_parse { s } {
#
# Builds the XML tree from the provided string
#
	upvar $s str

	set v [sxml_advance str ""]

	return [list root [lindex $v 0] "" [lindex $v 1]]
}

proc sxml_name { s } {

	return [lindex $s 0]
}

proc sxml_txt { s } {

	return [string trim [lindex $s 1]]
}

proc sxml_attr { s n } {

	set al [lindex $s 2]
	set n [string tolower $n]
	foreach a $al {
		if { [lindex $a 0] == $n } {
			return [lindex $a 1]
		}
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

###############################################################################

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
		msg "connecting to a VUEE model: node $Uart(NODE),\					host $Uart(HOST), port $Uart(PORT) ..."

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
			# it can be empty, in which case the node inherits
			# the frequency of its aggregator
			if [napin fr] {
				smerr "illegal frequency in collector $ix"
				return 0
			}
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

	# hot swap the tables #################################################

	set Converters $cnvts
	set Sensors $snsrs
	set Collectors $colls
	set Aggregators $aggts
	set OSSI $ossi

	array unset SBN
	array unset Nodes

	foreach no [array names nodes] {
		set Nodes($no) $nodes($no)
	}

	foreach no [array names sbn] {
		set SBN($no) $sbn($no)
	}

	msg "[llength $Aggregators] aggregators,\
		[llength $Collectors] collectors, [llength $Sensors] sensors,\
			[llength $Converters] snippets"

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

	seek $Files(SVAL,FD) [expr $s << 5]
	puts -nonewline $Files(SVAL,FD) [encode_sv $v $Time]
	flush $Files(SVAL,FD)
}

proc values_init { } {
#
# Initialize the values file
#
	global Files Time Sensors

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

proc exnum { str n } {
#
# Extracts $n nonneg numbers from the string
#
	set res ""

	while { $n } {
		if ![regexp "\[0-9\]+" $str num] {
			return ""
		}

		set loc [string first $num $str]
		set str [string range $str [expr $loc + [string length $num]] \
			end]
		incr n -1
		lappend res [expr $num]
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

###############################################################################

proc input_line { inp } {
#
# Handle line input from UART
#
	global Time

	set inp [string trim $inp]

	# write the line to standard output (we may remove this later)
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

	if ![regexp "(\[0-9\]+)\\)(.*)" $inp m aid inp] {
		# can't locate the aggregator number
		return
	}

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

	# there are 8 values altogether, but for now we shall extract only 2,
	# i.e., frequency and power level
	set nl [exnum $inp 2]

	if { $nl == "" } {
		# something wrong
		return
	}

	set fr [lindex $nl 0]
	set pl [lindex $nl 1]

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

	if ![regexp "(\[0-9\]+)\\) +via +(\[0-9\]+)(.*)"\
	    $inp mat cid vid inp] {
		return
	}

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

	# extract the first four values
	set nl [exnum $inp 4]
	if { $nl == "" } {
		return
	}

	set fr [lindex $nl 0]
	set pl [lindex $nl 3]

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

	if ![regexp "Agg +(\[0-9\]+) +slot: +(\[0-9\]+),\
	    +ts: +(\[^ \]+) +Col +(\[0-9\]+) +slot: +(\[0-9\]+), +ts:\
	    +(\[^ \]+) +(.+)" \
	    $inp mat aid asl ats cid csl cts inp] {
	    #
	    # agg_Id, agg_slot, agg_tstamp, col_Id, col_slot, col_tstamp
	    #
		msg "erroneous agg report line: $inp"
		return
	}

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

	# dynamic parameters
	set dp [lindex $co 3]
	# previous aggeregator
	set oai [lindex $dp 3]
	# last slot
	set osl [lindex $dp 1]
	if { $osl != "" && $osl == $csl } {
		# EEPROM full on the collector
		msg "eeprom full on collector $cid"
	}
	set dp [list $Time $csl $cts $oai]
	set Nodes($cid) [lrep $co 3 $dp]

	# associate the aggregator
	assoc_agg $aid $cid

	# extract sensor values
	set uc 0
	while 1 {

		if ![regexp -nocase "(\[a-z\]\[a-z0-9\]*): +(\[0-9\]+)"\
		    $inp mat sl value] {
			# all done
			break
		}

		# remove from the string
		set ix [expr [string first $mat $inp] + [string length $mat]]
		set inp [string range $inp $ix end]

		# locate the sensor
		set nf 1
		foreach s $SBN($cid) {
			set se [lindex $Sensors $s]
			if { [lindex $se 1] == $sl } {
				# found
				set nf 0
				break
			}
		}

		if $nf {
			msg "sensor $sl at collector $cid not found, ignored"
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

		if { !$nf } {
			# convert
			set co [lindex $cv 1]
			if [catch { eval $co } err] {
				msg "conversion failed for $sn, collector $cid:\
					$err"
				continue
			}
		}

		# the value is ready
		value_update $s $value
		data_out $cid $sl $sn $value
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

	# make sure our node is the master; repeat at 10 sec intervals until
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
	puts "       -c external_command_file, default is command"
	puts ""
	exit 99
}

set Uart(MODE) ""

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
	set Files(SVAL) "values"
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
	
log_open

uart_init input_line

loop
