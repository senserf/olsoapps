#!/bin/bash
freewrap econnect.tcl -i tool.ico
freewrap genimage.tcl -i mixer.ico
gcc -o esdreader esdreader.c
