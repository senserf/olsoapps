#!/bin/bash
#
#	Copyright 2002-2020 (C) Olsonet Communications Corporation
#	Programmed by Pawel Gburzynski & Wlodek Olesinski
#	All rights reserved
#
#	This file is part of the PICOS platform
#
#
freewrap econnect.tcl -i tool.ico
freewrap genimage.tcl -i mixer.ico
gcc -o esdreader esdreader.c
