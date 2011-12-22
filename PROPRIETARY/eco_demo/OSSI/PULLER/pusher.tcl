#!/bin/sh
###########################\
exec tclsh "$0" "$@"

### TEMPORARY - to be removed and incorporated into puller.cgi

proc msource { f } {
#
# Intelligent 'source'
#
	if ![catch { uplevel #0 source $f } ] {
		# found it right here
		return
	}

	set dir "Scripts"
	set lst ""

	for { set i 0 } { $i < 10 } { incr i } {
		set dir "../$dir"
		set dno [file normalize $dir]
		if { $dno == $lst } {
			# no progress
			break
		}
		if ![catch { uplevel #0 source [file join $dir $f] } ] {
			# found it
			return
		}
	}

	# failed
	puts stderr "Cannot locate file $f 'sourced' by the script."
	exit 99
}

msource xml.tcl
msource log.tcl

###############################################################################

package require xml 1.0
package require log 1.0

###############################################################################

proc abt { mes } {

	puts stderr $mes
	exit 99
}

proc read_xml_config { } {
#
# Read and parse the config file 
#
	global Files Params

	if [catch { open $Files(XMLC) r } xfd] {
		abt "cannot open configuration file $Files(XMLC): $xfd"
	}

	if [catch { read $xfd } xc] {
		catch { close $xfd }
		abt "cannot read configuration file $Files(XMLC): $xfd"
	}

	catch { close $xfd }

	if [catch { sxml_parse xc } xc] {
		abt "configuration file error: $xc"
	}

	set xc [sxml_child $xc "puller"]

	if { $xc == "" } {
		abt "no <puller> element in configuration file"
	}

	set el [sxml_child $xc "repository"]
	if { $el == "" } {
		abt "no <repository> element in configuration file"
	}

	# the URL
	set ul [sxml_txt $el]

	if ![regexp "http://(\[^/\]+)(/.+)" $ul junk Params(HOS) Params(REF)] {
		abt "URL format error in configuration file"
	}

	set Params(POR) [sxml_attr $el "port"]
	if [catch { expr $Params(POR) } Params(POR)] {
		set Params(POR) 80
	}

	set Params(UID) [sxml_attr $el "user"]
	if { $Params(UID) == "" } {
		abt "no user attribute in <repository>"
	}

	set el [sxml_child $xc "log"]
	set Files(LOG) [sxml_txt $el]
	if { $Files(LOG) == "" } {
		set Files(LOG) "pusher.log"
	}

	if [catch { expr [sxml_attr $el "size"] } Params(MLS)] {
		set Params(MLS) 1000000
	}

	if [catch { expr [sxml_attr $el "versions"] } Params(LVE)] {
		set Params(LVE) 4
	}
}

proc escape { txt } {

	array set CODES {
 		" "    %20
 		"#"    %23
 		"$"    %24
 		"%"    %25
 		"&"    %26
 		"/"    %2F
 		":"    %3A
 		";"    %3B
 		"<"    %3C
 		"="    %3D
 		">"    %3E
 		"?"    %3F
 		"@"    %40
 		"\["    %5B
 		"\\"    %5C
 		"\]"    %5D
 		"^"    %5E
 		"`"    %60
 		"{"    %7B
 		"|"    %7C
 		"}"    %7D
 		"~"    %7E
	}

	set out ""

	set len [string length $txt]

	for { set i 0 } { $i < $len } { incr i } {
		set c [string index $txt $i]
		if [info exists CODES($c)] {
			set c $CODES($c)
		}
		append out $c
	}

	return $out
}

proc expdate { } {

	set sc [expr [clock seconds] + 72 * 3600]

	return [escape [clock format $sc -format "%b %d, %Y"]]
}

proc sock_in { sok } {

	global Response Timeout

	if [catch { read $sok } res] {
		# disconnection
		catch { after cancel $Timeout }
		set Timeout "e"
		return
	}

	append Response $res

	if [regexp "<string \[^>\]+>(\[^<\]*)<" $Response junk Response] {
		set Response [string trim $Response]
		# done
		catch { after cancel $Timeout }
		set Timeout ""
	}
}

proc push_it { txt } {

	global Params MSN MID Response

#GET /EMS_Gateway/EMSGateway_v2_0.asmx/Message_Text?Msg_Id=string&Mobile_Id=string&Message=string&Expiration_DateTime_UTC=string&Access_Id=string HTTP/1.1
#Host: emsgateway.emstechnologies.ca

	set Response ""

	if [catch { socket $Params(HOS) $Params(POR) } sok] {
		log "connection to $Params(HOS) \[$Params(POR)\] failed: $sok"
		return
	}

	fconfigure $sok -buffering none -translation lf

	set req "GET $Params(REF)/Message_Text?Msg_Id=$MSN&Mobile_Id=$MID&Message=[escape $txt]&Expiration_DateTime_UTC=[expdate]&Access_Id=$Params(UID) HTTP/1.1\nHost: $Params(HOS)\n"

	if [catch { puts $sok $req } err] {
		log "cannot write to host $Params(HOS) \[$Params(POR)\]: $err"
		catch { close $sok }
		return
	}

	fconfigure $sok -blocking 0

	fileevent $sok readable "sock_in $sok"

	set Timeout [after 30000 "set Timeout t"]

	vwait Timeout

	catch { close $sok}

	if { $Timeout == "t" } {
		log "response timeout"
		return
	}

	if { $Timeout == "e" } {
		log "peer disconnection"
		return
	}

	puts $Response

}

set Files(XMLC) [lindex $argv 0]

if { $Files(XMLC) == "" } {
	# default XML datafile
	set Files(XMLC) "puller.xml"
}

read_xml_config
log_open $Files(LOG) $Params(MLS) $Params(LVE)

set MID ""
set MSN [expr [clock seconds] & 0x0000ffff]

while 1 {

	set line [string trim [gets stdin]]

	if { $line == "" } {
		continue
	}

	set cmd ""
	set arg ""
	regexp "(\[^ \t\]+)(.*)" $line junk cmd arg

	if { $MID == "" || $cmd == "id" } {
		if { $cmd != "id" } {
			puts "'id' command expected"
			continue
		}
		set arg [string trim $arg]
		if [catch { expr $arg } arg] {
			puts "illegal numeric identifier: $arg"
			continue
		}
		set MID $arg
		continue
	}

	push_it $line
}
