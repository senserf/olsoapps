#!/bin/bash
./mkexe.sh
cp /bin/cygwin1.dll .
candle econnect.wxs
light econnect.wixobj -ext WixUIExtension
rm -f cygwin1.dll
