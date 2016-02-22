/*----------------------------------------------------------------------------
 *      RL-ARM - RTX
 *----------------------------------------------------------------------------
 *      Name:    ADC.C
 *      Author:  Farid Mabrouk
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

void ADC_init(void){
	
  /* Power enable, Setup pin, enable and setup AD converter interrupt             */  
  PCONP        |= (1 << 12);                       /* Enable power to AD block    */  
  PINSEL1       = 0x4000;                          /* AD0.0 pin function select   */   
  AD0CR         = 0x01200301;                      /* Power up, PCLK/4, sel AD0.0 */  
}