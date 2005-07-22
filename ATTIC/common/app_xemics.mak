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
PP=..\..\PicOS\eCOG
SP=..\..\PicOS\kernel
LP=..\..\Libs\Lib
LCOMS=..\..\Libs\LibComms
LDIAG=..\..\Libs\LibDiag
LCONF=..\..\Libs\LibConf
LAPPS=.\LibApps
KP=.\KTMP

CC_FLAGS=-I $(KP) -I $(LP) -I $(LCOMS) -I $(LDIAG) -I $(LCONF) -I $(LAPPS)
CL_FLAGS=-pack $(PP)\cstartup -lib $(LP) -lib $(LCOMS) -lib $(LDIAG) -lib $(LCONF) -lib $(LAPPS) -o Image -map $(PP)\internal
DEL_CMD=erase
MOV_CMD=move
COP_CMD=copy

run.xpv : $(PP)\kernelirq.asm $(PP)\cstartup.asm $(PP)\internal.map $(KP)\radio.asm $(KP)\main.asm $(KP)\kernel.asm $(KP)\tcv.asm $(KP)\ethernet.asm app.asm
	@attrib -r $(PP)\cstartup.asm
	$(CL) $(CL_FLAGS) $(CC_FLAGS) $(PP)\kernelirq.asm $(PP)\cstartup.asm $(KP)\main.asm $(KP)\kernel.asm $(KP)\tcv.asm $(KP)\ethernet.asm $(KP)\radio.asm app.asm
	@attrib +r $(PP)\cstartup.asm

$(KP)\main.asm : $(PP)\main.c $(SP)\kernel.h $(PP)\sysio.h $(PP)/radio.h options.sys
	if not exist $(KP) mkdir $(KP)
	$(COP_CMD) $(PP)\main.c $(KP)
	$(COP_CMD) $(SP)\kernel.h $(KP)
	$(COP_CMD) $(PP)\sysio.h $(KP)
	$(COP_CMD) $(PP)\radio.h $(KP)
	$(COP_CMD) options.sys $(KP)
	$(CC) $(CC_FLAGS) $(KP)\main.c
	$(MOV_CMD) main.asm $(KP)

$(KP)\kernel.asm : $(SP)\kernel.c $(SP)\kernel.h $(PP)\sysio.h options.sys
	if not exist $(KP) mkdir $(KP)
	$(COP_CMD) $(SP)\kernel.c $(KP)
	$(COP_CMD) $(SP)\kernel.h $(KP)
	$(COP_CMD) $(PP)\sysio.h $(KP)
	$(COP_CMD) options.sys $(KP)
	$(CC) $(CC_FLAGS) $(KP)\kernel.c
	$(MOV_CMD) kernel.asm $(KP)

$(KP)\tcv.asm : $(PP)\tcv.c $(SP)\kernel.h $(PP)\sysio.h $(PP)\tcv.h $(PP)\tcvphys.h $(PP)\tcvplug.h options.sys
	if not exist $(KP) mkdir $(KP)
	$(COP_CMD) $(PP)\tcv.c $(KP)
	$(COP_CMD) $(SP)\kernel.h $(KP)
	$(COP_CMD) $(PP)\sysio.h $(KP)
	$(COP_CMD) $(PP)\tcv.h $(KP)
	$(COP_CMD) $(PP)\tcvphys.h $(KP)
	$(COP_CMD) $(PP)\tcvplug.h $(KP)
	$(COP_CMD) options.sys $(KP)
	$(CC) $(CC_FLAGS) $(KP)\tcv.c
	$(MOV_CMD) tcv.asm $(KP)

$(KP)\ethernet.asm : $(PP)\ethernet.c $(SP)\kernel.h $(PP)\sysio.h $(PP)\tcv.h $(PP)\ethernet.h $(PP)\tcvphys.h options.sys
	if not exist $(KP) mkdir $(KP)
	$(COP_CMD) $(PP)\ethernet.c $(KP)
	$(COP_CMD) $(SP)\kernel.h $(KP)
	$(COP_CMD) $(PP)\sysio.h $(KP)
	$(COP_CMD) $(PP)\tcv.h $(KP)
	$(COP_CMD) $(PP)\ethernet.h $(KP)
	$(COP_CMD) $(PP)\tcvphys.h $(KP)
	$(COP_CMD)  options.sys $(KP)
	$(CC) $(CC_FLAGS) $(KP)\ethernet.c
	$(MOV_CMD) ethernet.asm $(KP)

$(KP)\radio.asm : $(PP)\radio.c $(SP)\kernel.h $(PP)\sysio.h $(PP)\tcv.h $(PP)\radio.h $(PP)\tcvphys.h options.sys
        if not exist $(KP) mkdir $(KP)
        $(COP_CMD) $(PP)\radio.c $(KP)
        $(COP_CMD) $(SP)\kernel.h $(KP)
        $(COP_CMD) $(PP)\sysio.h $(KP)
        $(COP_CMD) $(PP)\tcv.h $(KP)
        $(COP_CMD) $(PP)\radio.h $(KP)
        $(COP_CMD) $(PP)\tcvphys.h $(KP)
        $(COP_CMD)  options.sys $(KP)
        $(CC) $(CC_FLAGS) $(KP)\radio.c
        $(MOV_CMD) radio.asm $(KP)

app.asm : app.c $(SP)\kernel.h $(PP)\sysio.h $(PP)\tcvphys.h $(PP)\tcvplug.h options.sys
	if not exist $(KP) mkdir $(KP)
	$(COP_CMD) $(SP)\kernel.h $(KP)
	$(COP_CMD) $(PP)\sysio.h $(KP)
	$(COP_CMD) $(PP)\tcvphys.h $(KP)
	$(COP_CMD) $(PP)\tcvplug.h $(KP)
	$(COP_CMD) options.sys $(KP)
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
	-$(DEL_CMD) KTMP\*
	rmdir KTMP
