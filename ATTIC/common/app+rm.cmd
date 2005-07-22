
> remove_custom SimNet


> reset ALL

Install the Simulator IO System.
> install_custom "..\..\simnets\debug\simnet.dll"

Configure the memory map.
> map_log 0 7ff rom
> map_log e800 efff ram
> map_log p'0 p'77ff rom

CACHE in cstartup.asm 
> map_log 1000 1400 ram

sdram (need be custom as physical spare SDRAM >32K in logical data space)
map_log 4000 bfff custom
>map_log 4000 bfff ram
SDRAM: mmu.ext_cs0_data[0-1]_[log,phy,size] read from coremem if needed

-------map some IO registers as custom------

LEDS: io.gp0_3_out
> map_log ffad custom

DUART: duart.[a-b]_[tx8,tx16,rx]
> map_log fea8 feaa custom
> map_log feb1 feb3 custom
DUART: duart.[a-b]_sts, duart.[a-b]_int_[dis,en]
> map_log fea4 fea6 custom
> map_log fead feaf custom

LCD: io.gp8_11_out
> map_log ffaf custom
LCD for speed: gp16_19_out, gp20_23_out
> map_log FFB1 FFB2 custom

CLOCK: ssm.cpu, tim.int_en1, tim.int_dis1 
> map_log ff72 custom
> map_log ff3b custom
> map_log ff3d custom
CLOCK support for RADIO XEMICS: tim.ctrl_[en,dis] -- to keep track of CNT1 on/off as a proxy for Txing/Rxing
> map_log ff1e ff1f custom

ETHER: {ADDR[0-2]_REG, DATA_REG, RXFIFO_REG} and MMU_CMD_REG
> map_log 3d82 3d84 custom
> map_log 3d80 custom
the other ETHER registers must be mapped as well to avoid trap write_none
> map_log 3d81 ram
> map_log 3d85 3d87 ram

BEEPER: ssm.tap_sel2, ssm.rst_set
> map_log ff77 custom
> map_log ff6a custom

RADIO: RFMI or VALERT: io.gp8_15_sts (Rxing), and rg.io.gp12_15_out (Txing)
> map_log ffb6 custom
> map_log ffb0 custom
RADIO: XEMICS: io.gp0_3_out (Txing, o/w LEDs, @ FFAD), io.gp8_15_sts (Rxing) 
+ for speed:
> map_log ffaa custom

turn off READ_X exception (read from unintialized data location)
> trap READ_X OFF

------------------

Load the eCOG code.
> load "image"

> go
