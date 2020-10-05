#!/bin/bash
#
#	Copyright 2002-2020 (C) Olsonet Communications Corporation
#	Programmed by Pawel Gburzynski & Wlodek Olesinski
#	All rights reserved
#
#	This file is part of the PICOS platform
#
#
./mkexe.sh
cp /bin/cygwin1.dll .
candle econnect.wxs
light econnect.wixobj -ext WixUIExtension
rm -f cygwin1.dll
