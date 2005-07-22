# Copyright (c) 2003 Olsonet Communication Corporation
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to
# deal in the Software without restriction, including without limitation the
# rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
# sell copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
# IN THE SOFTWARE.
#
CC=ecogncc
CL=ecogcl
PP=../../PicOS/eCOG
SP=../../PicOS/kernel
LP=../../Libs/Lib
LCOMS=../../Libs/LibComms
LDIAG=../../Libs/LibDiag
LCONF=../../Libs/LibConf
LAPPS=./LibApps
KP=./KTMP

CC_FLAGS=-I $(KP) -I $(LP) -I $(LCOMS) -I $(LDIAG) -I $(LCONF) -I $(LAPPS)
CL_FLAGS=-pack $(PP)/cstartup -lib $(LP) -lib $(LCOMS) -lib $(LDIAG) -lib $(LCONF) -lib $(LAPPS) -o Image -map $(PP)/internal
DEL_CMD=rm -f

run.xpv : $(PP)/kernelirq.asm $(PP)/cstartup.asm $(PP)/internal.map $(KP)/main.asm $(KP)/kernel.asm $(KP)/tcv.asm $(KP)/ethernet.asm $(KP)/gpioirq.asm app.asm
	$(CL) $(CL_FLAGS) $(CC_FLAGS) $(PP)/kernelirq.asm $(PP)/cstartup.asm $(KP)/main.asm $(KP)/kernel.asm $(KP)/tcv.asm $(KP)/ethernet.asm $(KP)/gpioirq.asm app.asm

$(KP)/main.asm : $(PP)/main.c $(SP)/kernel.h $(PP)/sysio.h options.sys
	mkdir -p $(KP)
	cp $(PP)/main.c $(SP)/kernel.h $(PP)/sysio.h options.sys $(KP)
	$(CC) $(CC_FLAGS) $(KP)/main.c 
	mv main.asm $(KP)

$(KP)/kernel.asm : $(SP)/kernel.c $(SP)/kernel.h $(PP)/sysio.h options.sys
	mkdir -p $(KP)
	cp $(SP)/kernel.c $(SP)/kernel.h $(PP)/sysio.h options.sys $(KP)
	$(CC) $(CC_FLAGS) $(KP)/kernel.c
	mv kernel.asm $(KP)

$(KP)/tcv.asm : $(PP)/tcv.c $(SP)/kernel.h $(PP)/sysio.h $(PP)/tcv.h $(PP)/tcvphys.h $(PP)/tcvplug.h options.sys
	mkdir -p $(KP)
	cp $(PP)/tcv.c $(SP)/kernel.h $(PP)/sysio.h $(PP)/tcv.h $(PP)/tcvphys.h $(PP)/tcvplug.h options.sys $(KP)
	$(CC) $(CC_FLAGS) $(KP)/tcv.c
	mv tcv.asm $(KP)

$(KP)/gpioirq.asm : $(PP)/gpioirq.c $(SP)/kernel.h $(PP)/sysio.h $(PP)/ethernet.h $(PP)/chipcon.h $(PP)/radio.h $(PP)/irq_ethernet.h $(PP)/irq_chipcon.h $(PP)/irq_xemics.h options.sys
	mkdir -p $(KP)
	cp $(PP)/gpioirq.c $(SP)/kernel.h $(PP)/sysio.h $(PP)/ethernet.h $(PP)/chipcon.h $(PP)/radio.h $(PP)/irq_ethernet.h $(PP)/irq_chipcon.h $(PP)/irq_xemics.h options.sys $(KP)
	$(CC) $(CC_FLAGS) $(KP)/gpioirq.c
	mv gpioirq.asm $(KP)

$(KP)/ethernet.asm : $(PP)/ethernet.c $(SP)/kernel.h $(PP)/sysio.h $(PP)/tcv.h $(PP)/ethernet.h $(PP)/tcvphys.h options.sys
	mkdir -p $(KP)
	cp $(PP)/ethernet.c $(SP)/kernel.h $(PP)/sysio.h $(PP)/tcv.h $(PP)/ethernet.h $(PP)/tcvphys.h options.sys $(KP)
	$(CC) $(CC_FLAGS) $(KP)/ethernet.c
	mv ethernet.asm $(KP)

app.asm : app.c $(SP)/kernel.h $(PP)/sysio.h $(PP)/tcvphys.h $(PP)/tcvplug.h options.sys
	mkdir -p $(KP)
	cp $(SP)/kernel.h $(PP)/sysio.h $(PP)/tcvphys.h $(PP)/tcvplug.h options.sys $(KP)
	$(CC) $(CC_FLAGS) app.c

clean :
	-$(DEL_CMD) *.lif
	-$(DEL_CMD) *.lib
	-$(DEL_CMD) *.lkr
	-$(DEL_CMD) *.sym
	-$(DEL_CMD) *.rom
	-$(DEL_CMD) *.asm
	-$(DEL_CMD) *.xdv
	-$(DEL_CMD) *.xpv
	rm -rf KTMP

share:
	scp -r * sheerness.cs.ualberta.ca:ECOGSHARE
