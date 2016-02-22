/*----------------------------------------------------------------------------
 *      Name:   SpeedControl.C
 *      Author: Farid Mabrouk
 *      Purpose: AMT7 example program
 *----------------------------------------------------------------------------
 *      This code is part of the RealView Run-Time Library.
 *      Copyright (c) 2004-2011 KEIL - An ARM Company. All rights reserved.
 *---------------------------------------------------------------------------*/

/******************************************************************************/
/* This file is part of the uVision/ARM development tools.                    */
/* Copyright (c) 2005-2006 Keil Software. All rights reserved.                */
/* This software may only be used under the terms of a valid, current,        */
/* end user licence from KEIL for a compatible version of KEIL software       */
/* development tools. Nothing else gives you the right to use this software.  */
/******************************************* ***********************************/
#include <stdio.h>
#include <LPC23xx.H>                    /* LPC23xx definitions                */
#include "motor.h"
#include <math.h>

/* Macro Definitions */ 
#define SPEED_UP       0x01
#define CRUISE         0x02
#define SPEED_DOWN     0x03
#define IDLE           0x04

static unsigned int delay_constant; 
static float  temp0,temp1,temp2,temp3; 
volatile static unsigned int flag,j;
volatile static char next_state;
volatile static unsigned int total_steps=4200;
volatile static	unsigned int denom;
volatile static unsigned int step;

/* Function that turns on requested LED                                       */
void LED_On (unsigned int num) {
  FIO2SET = (1 << num);
}

/* Function that turns off requested LED                                      */
void LED_Off (unsigned int num) {
  FIO2CLR = (1 << num);
}

 __irq void T0_IRQHandler (void) {
	
	int i;
	 
	j++;

	LED_On (0x07);
	
	for(i=0; i<120; i++)
	{  ; }
	
	LED_Off (0x07);
	
	if (j==4100)
	{
	T0TCR         = 0;                                 /* Timer0 Enable               */
  printf( "number of pulses is : %d \n",j);
  printf( "delay is : %d \n",delay_constant);
	next_state=0x4;
	}
	else
	{
	T0TCR         = 1;                                 /* Timer0 Enable               */
	T0MR0         =delay_constant;
	}
	
	T0IR          = 1;                                 /* Clear interrupt flag       */
	VICVectAddr   = 0; 
	flag=1;
	
}

float fastsqrt(float val) {
    long tmp = *(long *)&val;
    tmp -= 127L<<23; /* Remove IEEE bias from exponent (-2^23) */
    /* tmp is now an appoximation to logbase2(val) */
    tmp = tmp >> 1; /* divide by 2 */
    tmp += 127L<<23; /* restore the IEEE bias from the exponent (+2^23) */
    return *(float *)&tmp;
}

/* Function that initializes LEDs                                             */
void LED_Init(void) {
	
      PINSEL10 = 0;                         /* Disable ETM interface, enable LEDs */
      FIO2DIR  = 0x000000FF;                /* P2.0..7 defined as Outputs         */
      FIO2MASK = 0x00000000;
}

void SetupTimerInterrupt(void ){
	/* setup the timer interrupt */
  T0MCR         = 3;                             /* Interrupt and Reset on MR0  */
	VICVectAddr4  = (unsigned long ) T0_IRQHandler;/* Set Interrupt Vector        */
	VICVectCntl4  = 15;                            /* use it for Timer0 Interrupt */
	VICIntEnable  = (1  << 4);
	
}
	void Calculate_C0(){
		
		/*C0 equation: C0=frequency*sqrt(2*motor_step_angle/angular_accel)*/
	step=0;
	temp0=2*motor_step_angle;
	temp0=temp0/angular_accel;
	temp0=fastsqrt(temp0);
	temp0=temp0*frequency;
	}
int main (void)
 { 
	 
	LED_Init();                                    /* LED Initialization         */ 
  SetupTimerInterrupt();
  Calculate_C0();

	/*Cn equation: Cn= (Cn-1)-(2*Cn-1/(4*step+1))*/	
	step++;
	denom=(step<<2)+1;
	temp1=(temp0+temp0)/denom;
	temp0=temp0- temp1;
	
	/* normalization so that delays are obtained in Microseconds */
	temp3=ceil(temp0/12);
	delay_constant=temp3;	
	flag=1;
	
  T0TCR         = 1;                             /* Timer0 Enable               */
  T0MR0         = delay_constant;
	next_state=0x1;
	
  while(1)
   	{
      if(flag)
			{
				flag=0;
				switch(next_state)
				{
					case SPEED_UP:
										
							 step++;
							 if(step==1200)
							   {
								 next_state=0x2;
								 }
							 denom=(step<<2)+1;
							 temp1=(temp0+temp0)/denom;
							 temp0=temp0- temp1;

							 /* normalization so that delays are obtained in Microseconds */
							 temp3=ceil(temp0/12);
							 delay_constant=temp3;	
						   break;
								 
					case CRUISE:
							 step++;
							 if(step==3001)
							 {
							 delay_constant=temp3;
							 next_state=0x3;
							 }
							 break;
					case SPEED_DOWN:
					
							 /*Cn equation: Cn= (Cn-1)-(2*Cn-1/4*(step-total_steps)+1)*/	
							 denom= total_steps-step;  
							 denom=(denom<<2)-1;
							 temp1=(temp0+temp0)/denom;
							 temp0=temp0+temp1;
					
							 /* normalization so that delays are obtained in Microseconds */
							 temp3=ceil(temp0/12);
							 delay_constant=temp3;
					
							 step++;
							 if(step==4199)
							 {
							 next_state=0x4;
               printf("exit\n" );
							 }
							 break;
					case IDLE:
						   printf("Motor is in Idle state\n");
					     break;
					 default :
              printf("exit\n" );
					}
		}
	}
}
