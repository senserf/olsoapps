#!/bin/sh
cd PICTURES

../cbm.tcl p1.bmp p1.nok
../cbm.tcl p2.bmp p2.nok
../cbm.tcl p4.bmp p4.nok
../cbm.tcl p8.bmp p8.nok
../cbm.tcl wall.bmp wallpaper.nok
../cbm.tcl event77.bmp e77.nok
../cbm.tcl ad88.bmp a88.nok
../cbm.tcl p12.bmp c12.nok
../cbm.tcl p14.bmp c14.nok
../cbm.tcl p18.bmp c18.nok
../cbm.tcl p24.bmp c24.nok
../cbm.tcl p28.bmp c28.nok
../cbm.tcl p48.bmp c48.nok
../cbm.tcl p124.bmp c124.nok
../cbm.tcl p128.bmp c128.nok
../cbm.tcl p148.bmp c148.nok
../cbm.tcl p248.bmp c248.nok

cd ../FONTS
gcc -o mkfonts fontfile.c
./mkfonts > fonts.nok
rm mkfonts*
cd ..

./mkeeprom.tcl -i PICTURES FONTS/fonts.nok noombuzz1.xml eeprom1.nok
./mkeeprom.tcl -i PICTURES FONTS/fonts.nok noombuzz2.xml eeprom2.nok
./mkeeprom.tcl -i PICTURES FONTS/fonts.nok noombuzz4.xml eeprom4.nok
./mkeeprom.tcl -i PICTURES FONTS/fonts.nok noombuzz8.xml eeprom8.nok
./mkeeprom.tcl -i PICTURES FONTS/fonts.nok noombuzz_booth.xml eeprom_booth.nok

