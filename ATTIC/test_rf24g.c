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
 *
 *  CE		R		P6.6	GP6	OUT
 *  CS		R		P6.7	GP7	OUT
 *  CLK1	R		P1.1	CFG1	OUT
 *  DR1		R		P1.2	CFG2	IN (Data Ready Interrupt)
 *  DATA	R		P2.3	GP1	IN/OUT
 */

heapmem {100};

void ini_regs () {

	_BIC (P6OUT, 0xf8);
	_BIC (P2OUT, 0x18);
	_BIC (P1OUT, 0x07);

	_BIS (P6DIR, 0xf8);
	_BIS (P1DIR, 0x02);

	// Disable on board radio
	_BIC (P2OUT, 0x01);
	_BIS (P2DIR, 0x01);
	_BIC (P5OUT, 0x03);
	_BIS (P5DIR, 0x03);
}

#define	b_int_p			(P1IN & 0x01)

#define	b_clk_up		do { _BIS (P6OUT, 0x20); udelay (2); } while (0)
#define	b_clk_down		_BIC (P6OUT, 0x20)
#define	b_ce_up			_BIS (P6OUT, 0x08)
#define	b_ce_down		_BIC (P6OUT, 0x08)
#define	b_cs_up			_BIS (P6OUT, 0x10)
#define	b_cs_down		_BIC (P6OUT, 0x10)

#define	b_data_out		_BIS (P2DIR, 0x10)
#define	b_data_in		_BIC (P2DIR, 0x10)
#define	b_data_up		_BIS (P2OUT, 0x10)
#define	b_data_down		_BIC (P2OUT, 0x10)
#define	b_data_val		(P2IN & 0x10)
	
#define	a_int_p			(P1IN & 0x04)

#define	a_clk_up		do { _BIS (P1OUT, 0x02); udelay (2); } while (0)
#define	a_clk_down		_BIC (P1OUT, 0x02)
#define	a_ce_up			_BIS (P6OUT, 0x40)
#define	a_ce_down		_BIC (P6OUT, 0x40)
#define	a_cs_up			_BIS (P6OUT, 0x80)
#define	a_cs_down		_BIC (P6OUT, 0x80)

#define	a_data_out		_BIS (P2DIR, 0x08)
#define	a_data_in		_BIC (P2DIR, 0x08)
#define	a_data_up		_BIS (P2OUT, 0x08)
#define	a_data_down		_BIC (P2OUT, 0x08)
#define	a_data_val		(P2IN & 0x08)

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

void boot_up0(void);
void configure_receiver0(void);
void configure_transmitter0(void);
void transmit_data0(void);
void receive_data0(void);

void boot_up1(void);
void configure_receiver1(void);
void configure_transmitter1(void);
void transmit_data1(void);
void receive_data1(void);

void do_main()
{
    int i;
    uns16 elapsed_time;

    counter = 0;
    
    while (1) {

#if 0
    	boot_up0();
	udelay (100);

	for (i = 0; i < 20; i++) {

	        counter++;
        
	        data_array[0] = 0x12;
	        data_array[1] = 0x34;
	        data_array[2] = 0xAB;
	        data_array[3] = counter;

	        diag ("Sending data B->A ...");
	        transmit_data0();
		    
		for (elapsed_time = 1; elapsed_time != 0; elapsed_time++) {
			if (a_int_p)
				break;
			udelay (5);
		}

	        if(a_int_p)
	            receive_data0();
	        else
	            diag ("No data found!");
        
	        mdelay (1000);
	}
#endif

    	boot_up1();
	udelay (100);

	for (i = 0; i < 20; i++) {

	        counter++;
        
	        data_array[0] = 0x12;
	        data_array[1] = 0x34;
	        data_array[2] = 0xAB;
	        data_array[3] = counter;

	        diag ("Sending data ...");
	        transmit_data1();
		    
#if 0
		for (elapsed_time = 1; elapsed_time != 0; elapsed_time++) {
			if (b_int_p)
				break;
			udelay (5);
		}

	        if(b_int_p)
	            receive_data1();
	        else
	            diag ("No data found!");
#endif
        
	        mdelay (1000);
	}
    }
}

void boot_up0(void)
{

    diag ("\n\rRF-24G Testing B->A:");
    configure_transmitter0();
    configure_receiver0();

}

void boot_up1(void)
{

    diag ("\n\rRF-24G Testing A->B:");
    configure_transmitter1();
    configure_receiver1();


}

void receive_data0(void)
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
    
    if (a_int_p == 0)
        diag ("DR went low");
    
    diag ("Data Received:");
    diag("[0] : %x", data_array[0]);
    diag("[1] : %x", data_array[1]);
    diag("[2] : %x", data_array[2]);
    diag("[3] : %x", data_array[3]);

    a_ce_up;
}

void receive_data1(void)
{
    uns8 i, j, temp;

    b_data_in;
    b_ce_down;
    // b_int_c;

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
		if (b_data_val)
			temp |= 1;
		b_clk_up;
		b_clk_down;
	}
        data_array[i] = temp; //Store this byte
    }
    
    if (b_int_p == 0)
        diag ("DR went low");
    
    diag ("Data Received:");
    diag("[0] : %x", data_array[0]);
    diag("[1] : %x", data_array[1]);
    diag("[2] : %x", data_array[2]);
    diag("[3] : %x", data_array[3]);

    b_ce_up;
}

void transmit_data0(void)
{
    uns8 i, j, temp, rf_address;

    b_data_out;
    b_ce_up;

    //Clock in address
    // rf_address = 0b.1110.0111; //Power-on Default for all units (on page 11)
    rf_address = 0xE7;

    for(i = 0 ; i < 8 ; i++)
    {
	if ((rf_address & 0x80))
		b_data_up;
	else
		b_data_down;
	b_clk_up;
	b_clk_down;
        rf_address <<= 1;
    }
    
    //Clock in the data_array
    for(i = 0 ; i < 4 ; i++) //4 bytes
    {
        temp = data_array[i];
        
        for(j = 0 ; j < 8 ; j++) //One bit at a time
        {
		if ((temp & 0x80))
			b_data_up;
		else
			b_data_down;
		b_clk_up;
		b_clk_down;
	        temp <<= 1;
        }
    }

    b_ce_down;
}

void transmit_data1(void)
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
}

void configure_receiver0(void)
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

    diag ("RX Configuration finished");
}    

void configure_receiver1(void)
{
    uns8 i;
    unsigned long config_setup;

    b_data_out;
    b_ce_down;
    b_cs_up;

    udelay (7);

    // config_setup = 0b.0010.0011.0110.1110.0000.0101; //Look at pages 13-15 for more bit info
    config_setup = 0x236e05;

    for(i = 0 ; i < 24 ; i++)
    {
	if ((config_setup & 0x800000))
		b_data_up;
	else
		b_data_down;
	b_clk_up;
	b_clk_down;
        config_setup <<= 1;
    }

    b_ce_down;
    b_cs_down;

    udelay (7);

    b_ce_up;
    b_cs_down;

    diag ("RX Configuration finished");
}    

void configure_transmitter0(void)
{
    uns8 i;
    unsigned long config_setup;

    //Config Mode
    b_data_out;
    b_ce_down;
    b_cs_up;

    udelay (7);
    
    //Delay of 5us from CS to Data (page 30) is taken care of by the for loop
    
    //Setup configuration word
    // config_setup = 0b.0010.0011.0110.1110.0000.0100; //Look at pages 13-15 for more bit info
    config_setup = 0x236e04;

    for(i = 0 ; i < 24 ; i++)
    {
	if ((config_setup & 0x800000))
		b_data_up;
	else
		b_data_down;
	b_clk_up;
	b_clk_down;
        config_setup <<= 1;
    }
    
    //Configuration is actived on falling edge of CS (page 10)
    b_ce_up;
    b_cs_down;

    diag ("TX Configuration finished");
}

void configure_transmitter1(void)
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

    diag ("TX Configuration finished");
}

