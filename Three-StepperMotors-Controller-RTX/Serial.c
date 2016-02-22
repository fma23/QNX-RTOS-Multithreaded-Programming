/*----------------------------------------------------------------------------
 *      RL-ARM - RTX
 *----------------------------------------------------------------------------
 *      Name:   SpeedControl.C
 *      Author: Farid Mabrouk
 *      Purpose: RTX example program
 *----------------------------------------------------------------------------
 *      This code is part of the RealView Run-Time Library.
 *      Copyright (c) 2004-2011 KEIL - An ARM Company. All rights reserved.
 *---------------------------------------------------------------------------*/

#include <RTL.h>
#include <LPC23xx.H>                    /* LPC23xx definitions               */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

/*----------------------------------------------------------------------------
 *       serial_init: initialize serial interface
 *---------------------------------------------------------------------------*/
void serial_init (void) {

	//UART1
	PINSEL0 |= 0x40000000;                /*  Enable TxD1 in  P0.15   */
	PINSEL1 |= 0x00000001;                 /*  Enable RxD1 in P0.16   */
	U1LCR = 0x83;                          /* 8 bits, no Parity, 1 Stop bit */
	U1DLM = 0x00;
	U1DLL = 78;                             /* 9600 Baud Rate @ 12MHz Clock */
	U1FDR   = 0;                            /* Fractional divider not used     */
	U1LCR = 0x03;                           /* DLAB = 0 */
	U1FCR = 0x07;                           /* enable TX amp RX FIFO ,clears TX amp RX FIFO */
}

char sendchar (char character){
	
	 char c;  
	 c=character;
	 
	 while (!(U1LSR & 0x20)){};	
	 return (U1THR=c);
}
void SendInteger (int integer){
	
	int x; 
	static const char dec[] = "0123456789";
  unsigned int div_val = 100000;

	 x=integer;
	
  if (x < 0){
    x = - x;
   sendchar('-');
  }
  while (div_val > 1 && div_val > x)
    div_val /= 10;
  do{
    sendchar (dec[x / div_val]);
    x %= div_val;
    div_val /= 10;
   }while(div_val);	
}