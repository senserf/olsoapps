/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2005                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "sysio.h"

/* Raw test for RF24G. Trying to replicate the official example from PIC */

/* PINS:
 *
 *  RF24G                       MSP430Fxxx
 * ===========================================
 *  CE		X		P6.3	GP3	OUT
 *  CS		X		P6.4	GP4	OUT
 *  CLK1	X		P6.5	GP5	OUT
 *  CLK2					GND
 *  DR1		X		P1.0	CFG0	IN (Data Ready Interrupt)
 *  DR2						PULL DOWN?
 *  DOUT2					PULL DOWN?
 *  DATA	X		P2.4	GP2	IN/OUT P6.6
 */

heapmem {100};

void ini_regs () {

	_BIC (P6OUT, 0x78);
	_BIC (P3OUT, 0xaa);	// LEDS
	_BIC (P1OUT, 0x01);
	_BIS (P3DIR, 0xaa);	// LEDS
	_BIS (P6DIR, 0x38);
}

#define	a_int_p			(P1IN & 0x01)

#define	a_clk_up		do { _BIS (P6OUT, 0x20); udelay (2); } while (0)
#define	a_clk_down		_BIC (P6OUT, 0x20)
#define	a_ce_up			_BIS (P6OUT, 0x08)
#define	a_ce_down		_BIC (P6OUT, 0x08)
#define	a_cs_up			_BIS (P6OUT, 0x10)
#define	a_cs_down		_BIC (P6OUT, 0x10)

#define	a_data_out		_BIS (P6DIR, 0x40)
#define	a_data_in		_BIC (P6DIR, 0x40)
#define	a_data_up		_BIS (P6OUT, 0x40)
#define	a_data_down		_BIC (P6OUT, 0x40)
#define	a_data_val		(P6IN & 0x40)

#define	RECEIVE	1

void do_main ();

process (root, int)


  entry (0)

        ini_regs ();

	do_main ();
	proceed (0);

endprocess (1)

typedef unsigned char uns8;
typedef unsigned int uns16;


uns8 data_array[4];
uns8 counter;

void boot_up(void);
void configure_receiver(void);
void configure_transmitter(void);
void transmit_data(void);
int receive_data(void);
void blink (int, int);

void do_main()
{
    int i;
    uns16 elapsed_time;

    counter = 0;

    if (RECEIVE) {
	while (1) {
    		configure_receiver();
		blink (0, 4);
		for (i = 0; i < 8; i++) {
			for (elapsed_time = 1; elapsed_time; elapsed_time++) {
				if (a_int_p) {
					if (receive_data ()) {
						blink (1, 3);
						mdelay (200);
					} else {
						blink (1, 1);
						mdelay (200);
					}
				}
			}
		}
	}
    } else {
	while (1) {
		configure_transmitter();
		blink (0, 4);
		for (i = 0; i < 32; i++) {

		        counter++;
        
		        data_array[0] = 0x12;
		        data_array[1] = 0x34;
		        data_array[2] = 0xAB;
		        data_array[3] = counter;

		        transmit_data();
			blink (1, 2);
			mdelay (200);
		}
	}
    }
}

int receive_data(void)
{
    uns8 i, j, temp;

    a_data_in;
    a_ce_down;
    // a_int_c;

    data_array[0] = 0x00;
    data_array[1] = 0x00;
    data_array[2] = 0x00;
    data_array[3] = 0x00;

    //Clock in data, we are setup for 32-bit payloads
    for(i = 0 ; i < 4 ; i++) //4 bytes
    {
        for(j = 0 ; j < 8 ; j++) //8 bits each
        {
            	temp <<= 1;
		if (a_data_val)
			temp |= 1;
		a_clk_up;
		a_clk_down;
	}
        data_array[i] = temp; //Store this byte
    }
/* 
    if (a_int_p == 0)
	_BIS (P3OUT, 0x80);
    else
	_BIC (P3OUT, 0x80);
*/

    a_ce_up;

    if (data_array [0] == 0x12 && data_array [1] == 0x34) {
	return 1;
    }

    return 0;
}

void blink (int d, int t) {

	d = d ? 0x80 : 0x20;

	while (t--) {
		mdelay (125);
		_BIS (P3OUT, d);
		mdelay (125);
		_BIC (P3OUT, d);
	}
}

void transmit_data(void)
{
    uns8 i, j, temp, rf_address;

    a_data_out;
    a_ce_up;

    //Clock in address
    // rf_address = 0b.1110.0111; //Power-on Default for all units (on page 11)
    rf_address = 0xE7;

    for(i = 0 ; i < 8 ; i++)
    {
	if ((rf_address & 0x80))
		a_data_up;
	else
		a_data_down;
	a_clk_up;
	a_clk_down;
        rf_address <<= 1;
    }
    
    //Clock in the data_array
    for(i = 0 ; i < 4 ; i++) //4 bytes
    {
        temp = data_array[i];
        
        for(j = 0 ; j < 8 ; j++) //One bit at a time
        {
		if ((temp & 0x80))
			a_data_up;
		else
			a_data_down;
		a_clk_up;
		a_clk_down;
	        temp <<= 1;
        }
    }

    a_ce_down;
    mdelay (500);
}

void configure_receiver(void)
{
    uns8 i;
    unsigned long config_setup;

    a_data_out;
    a_ce_down;
    a_cs_up;

    udelay (7);

    // config_setup = 0b.0010.0011.0110.1110.0000.0101; //Look at pages 13-15 for more bit info
    config_setup = 0x236e05;

    for(i = 0 ; i < 24 ; i++)
    {
	if ((config_setup & 0x800000))
		a_data_up;
	else
		a_data_down;
	a_clk_up;
	a_clk_down;
        config_setup <<= 1;
    }

    a_ce_down;
    a_cs_down;

    udelay (7);

    a_ce_up;
    a_cs_down;

}    

void configure_transmitter(void)
{
    uns8 i;
    unsigned long config_setup;

    //Config Mode
    a_data_out;
    a_ce_down;
    a_cs_up;

    udelay (7);
    
    //Delay of 5us from CS to Data (page 30) is taken care of by the for loop
    
    //Setup configuration word
    // config_setup = 0b.0010.0011.0110.1110.0000.0100; //Look at pages 13-15 for more bit info
    config_setup = 0x236e04;

    for(i = 0 ; i < 24 ; i++)
    {
	if ((config_setup & 0x800000))
		a_data_up;
	else
		a_data_down;
	a_clk_up;
	a_clk_down;
        config_setup <<= 1;
    }
    
    //Configuration is actived on falling edge of CS (page 10)
    a_ce_up;
    a_cs_down;

}
