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


void DAC_init(void){
	
	PCLKSEL0 = 0;                        /* bit 23:22 PCLK_DAC is set to PCCLK/4*/
	DACR  =0;
	PINSEL1 |=0x00200000;		
}