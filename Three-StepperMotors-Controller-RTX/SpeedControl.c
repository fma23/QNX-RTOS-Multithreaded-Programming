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

/***********   Motor1 Variables   *******************/
volatile unsigned int M1delay_constant; 
static float  M1temp0,M1temp1,M1temp3; 
volatile static unsigned char M1flag,M1j;
volatile static	unsigned int M1denom;
volatile static unsigned int M1step=1;

unsigned int M1SegmentA1; 
unsigned int M1SegmentB1;
unsigned int M1total_steps;
unsigned int M1MaxSpeed;                      /* in rad/sec     */
float M1StepAngle;                            /* in rad         */
unsigned int M1Acceleration;
unsigned int M1Decceleration;

/***********  Motor2 Variables    *******************/
volatile unsigned int M2delay_constant; 
static float  M2temp0,M2temp1,M2temp3; 
volatile static unsigned char M2flag,M2j;
volatile static	unsigned int M2denom;
volatile static unsigned int M2step=1;

unsigned int M2SegmentA1; 
unsigned int M2SegmentB1;
unsigned int M2total_steps;
unsigned int M2MaxSpeed;                      /* in rad/sec     */
float M2StepAngle;                            /* in rad         */
unsigned int M2Acceleration;
unsigned int M2Decceleration;

/***********  Motor3 Variables    *******************/
volatile unsigned int M3delay_constant; 
static float  M3temp0,M3temp1,M3temp3; 
volatile static unsigned char M3flag,M3j;
volatile static	unsigned int M3denom;
volatile static unsigned int M3step=1;

unsigned int M3SegmentA1; 
unsigned int M3SegmentB1;
unsigned int M3total_steps;
unsigned int M3MaxSpeed;                      /* in rad/sec     */
float M3StepAngle;                            /* in rad         */
unsigned int M3Acceleration;
unsigned int M3Decceleration;

/********************************************************************/
unsigned int Frequency; 

extern void serial_init (void);
extern char sendchar (char character);
extern void SendInteger (int integer);
extern void motors_parameters_init(void);
extern void motors_calculations(void);

void T0_IRQHandler (void) __irq;
void T1_IRQHandler (void) __irq;
void T2_IRQHandler (void) __irq;

OS_TID  t_SMotor1;                /* start task      */
OS_TID	t_SMotor2;                /* start task      */
OS_TID	t_SMotor3;                /* start task      */

/* Function that initializes LEDs                                             */
void LED_Init(void) {
	
      PINSEL10 = 0;                         /* Disable ETM interface, enable LEDs */
      FIO2DIR  = 0x000000FF;                /* P2.0..7 defined as Outputs         */
      FIO2MASK = 0x00000000;
}

/* Function that turns on requested LED                                       */
void LED_On (unsigned int num) {
  FIO2SET = (1 << num);
}

/* Function that turns off requested LED                                      */
void LED_Off (unsigned int num) {
  FIO2CLR = (1 << num);
}

float fastsqrt(float val) {
	
    long tmp = *(long *)&val;
    tmp -= 127L<<23; /* Remove IEEE bias from exponent (-2^23) */
    /* tmp is now an appoximation to logbase2(val) */
    tmp = tmp >> 1; /* divide by 2 */
    tmp += 127L<<23; /* restore the IEEE bias from the exponent (+2^23) */
    return *(float *)&tmp;
}

/*----------------------------------------------------------------------------
 *        Function for speed profile for MOTOR1
 *---------------------------------------------------------------------------*/
void M1Calculate_C0()
{
	/*C0 equation: C0=frequency*sqrt(2*motor_step_angle/angular_accel)*/
	M1temp0=2*M1StepAngle;
	M1temp0=M1temp0/M1Acceleration;
	M1temp0=fastsqrt(M1temp0);
	M1temp0=M1temp0*Frequency;
}
	
void M1Calculate_CnUp(){
	
	/*Cn equation: Cn= (Cn-1)-(2*Cn-1/(4*step+1))*/	
	   M1denom=(M1step<<2)+1;
	   M1temp1=(M1temp0+M1temp0)/M1denom;
	   M1temp0= M1temp0- M1temp1;
	
	   M1temp3=10*M1temp0;
		 
	/* normalization so that delays are obtained in Microseconds */
	   M1delay_constant=ceil(M1temp3/12);
	}
/*--------------------------------------------------------------------------*/
void M1Calculate_CnDwn()
	{	
	 /*Cn equation: Cn= (Cn-1)-(2*Cn-1/4*(step-total_steps)+1)*/	
		 M1denom= M1total_steps-M1step;  
		 M1denom=(M1denom<<2)-1;
		 M1temp1=(M1temp0+M1temp0)/M1denom;
		 M1temp0=M1temp0+M1temp1;
		 
		 M1temp3=10*M1temp0;

	  /* normalization so that delays are obtained in Microseconds */
    M1delay_constant=ceil(M1temp3/12);
		
		
	}
/*----------------------------------------------------------------------------
 *        Function for speed profile for MOTOR2
 *---------------------------------------------------------------------------*/
void M2Calculate_C0()
{
	/*C0 equation: C0=frequency*sqrt(2*motor_step_angle/angular_accel)*/
	M2temp0=2*M2StepAngle;
	M2temp0=M2temp0/M2Acceleration;
	M2temp0=fastsqrt(M2temp0);
	M2temp0=M2temp0*Frequency;
}
	
void M2Calculate_CnUp(){
		
	/*Cn equation: Cn= (Cn-1)-(2*Cn-1/(4*step+1))*/	
	   M2denom=(M2step<<2)+1;
	   M2temp1=(M2temp0+M2temp0)/M2denom;
	   M2temp0= M2temp0- M2temp1;
	   M2temp3=10*M2temp0;
		 
	/* normalization so that delays are obtained in Microseconds */
	   M2delay_constant=ceil(M2temp3/12);
	}
/*--------------------------------------------------------------------------*/
void M2Calculate_CnDwn()
	{	
	 /*Cn equation: Cn= (Cn-1)-(2*Cn-1/4*(step-total_steps)+1)*/	
		 M2denom= M2total_steps-M2step;  
		 M2denom=(M2denom<<2)-1;
		 M2temp1=(M2temp0+M2temp0)/M2denom;
		 M2temp0=M2temp0+M2temp1; 
		 M2temp3=10*M2temp0;

	  /* normalization so that delays are obtained in Microseconds */
    M2delay_constant=ceil(M2temp3/12);	
	}
/*----------------------------------------------------------------------------
 *        Function for speed profile for MOTOR3
 *---------------------------------------------------------------------------*/
void M3Calculate_C0()
{
	/*C0 equation: C0=frequency*sqrt(2*motor_step_angle/angular_accel)*/
	M3temp0=2*M3StepAngle;
	M3temp0=M3temp0/M3Acceleration;
	M3temp0=fastsqrt(M3temp0);
	M3temp0=M3temp0*Frequency;
}
	
void M3Calculate_CnUp(){
		
	/*Cn equation: Cn= (Cn-1)-(2*Cn-1/(4*step+1))*/	
	   M3denom=(M3step<<2)+1;
	   M3temp1=(M3temp0+M3temp0)/M3denom;
	   M3temp0= M3temp0- M3temp1;
	   M3temp3=10*M2temp0;
		 
	/* normalization so that delays are obtained in Microseconds */
	   M3delay_constant=ceil(M3temp3/12);
	}
/*--------------------------------------------------------------------------*/
void M3Calculate_CnDwn()
	{	
	 /*Cn equation: Cn= (Cn-1)-(2*Cn-1/4*(step-total_steps)+1)*/	
		 M3denom= M3total_steps-M3step;  
		 M3denom=(M3denom<<2)-1;
		 M3temp1=(M3temp0+M3temp0)/M3denom;
		 M3temp0=M3temp0+M3temp1; 
		 M3temp3=10*M3temp0;

	   /* normalization so that delays are obtained in Microseconds */
     M3delay_constant=ceil(M3temp3/12);	
	}
/*----------------------------------------------------------------------------
 *        Timer0 Interrupt for MOTOR1
 *---------------------------------------------------------------------------*/
void Timer0_Int(void)
	{
	/* setup the timer interrupt */
  T0MCR         = 3;                             /* Interrupt and Reset on MR0  */
	VICVectAddr4  = (unsigned long ) T0_IRQHandler;/* Set Interrupt Vector        */
	VICVectCntl4  = 15;                            /* use it for Timer0 Interrupt */
	VICIntEnable  = (1  << 4);
	}
/*-------------------------------------------------------------------------------*/	
	void T0_IRQHandler (void) __irq  {
	
	int i; 
		
	LED_On (0x07);
	
	for(i=0; i<120; i++)
	{ ; }
	
	LED_Off (0x07);
  
	isr_evt_set (0x0011, t_SMotor1);              /* send event signal to clock task  */
	
	T0IR          = 1;                            /* Clear interrupt flag       */
	VICVectAddr   = 0;                            /* Dummy write to signal end of interrupt */
}
/*----------------------------------------------------------------------------
 *        Timer1 Interrupt for MOTOR2
 *---------------------------------------------------------------------------*/
void Timer1_Int(void)
	{
	/* setup the timer interrupt */
  T1MCR         = 0x18;                             /* Interrupt and Reset on MR1  */
	VICVectAddr5  = (unsigned long ) T1_IRQHandler;/* Set Interrupt Vector        */
	VICVectCntl5  = 0x25;                            /* use it for Timer1 Interrupt */
	VICIntEnable  = (1  << 5);
	}
/*-------------------------------------------------------------------------------*/	
	
	void T1_IRQHandler (void) __irq  {
	
	int i; 
		
	LED_On (0x06);
	
	for(i=0; i<120; i++)
	{ ; }
	
	LED_Off (0x06);
	
	isr_evt_set (0x0111, t_SMotor2);              /* send event signal to clock task  */
	
	T1IR          = 2;                           /* Clear interrupt flag       */
	VICVectAddr   = 0; 
}
/*----------------------------------------------------------------------------
 *        Timer2 Interrupt for MOTOR3
 *---------------------------------------------------------------------------*/
void Timer2_Int(void)
	{
	/* setup the timer interrupt */
	T2MCR          = 0x000000C0;                          /* Interrupt and Reset on MR2  */
	VICVectAddr26  = (unsigned long)T2_IRQHandler;        /* Set Interrupt Vector        */
	VICVectCntl26  = 6;                                  /* use it for Timer2 Interrupt */
	VICIntEnable   = (1  << 26);
	}
/*-------------------------------------------------------------------------------*/	
void T2_IRQHandler (void) __irq  {
	
	int i; 
	
	LED_On (0x05);
	
	for(i=0; i<120; i++)
	{  ; }
	
	LED_Off (0x05);
  
	isr_evt_set (0x1111, t_SMotor3);                /* send event signal to clock task  */

	
	T2IR          = 4;                           /* Clear interrupt flag       */                          /* Clear interrupt flag       */
	VICVectAddr   = 0; 
}
/*----------------------------------------------------------------------------
 *        Task SMotor1 
 *---------------------------------------------------------------------------*/
__task void SMotor1 (void) 
{
	
	M1Calculate_C0();                      /* Calculate C0 */
	M1Calculate_CnUp();                    /* Calculate Cn */
	
	Timer0_Int();                          /*initialize timer0 */
 
	while(1)
   	{	  
			  T0TCR         = 1;                             /* Timer0 Enable */
	      T0MR0         = M1delay_constant;
			  
			  M1step=M1step+1;
			
				if (M1step <=M1SegmentA1)
				{	
				M1Calculate_CnUp();	
				}
				else if (M1step>M1SegmentA1 && M1step <=M1SegmentB1)
				{
				/*constant speed*/
				}
				else if (M1step >M1SegmentB1)
				{
				M1Calculate_CnDwn();
				}			
				
				if (M1step ==(M1total_steps-1))
				{
        T0MCR=0X00000004;                        /* stop on T0 */
				T0TCR         = 0;                       /* Timer0 disable */
				os_tsk_delete_self ();
				}
		
				os_evt_wait_and (0x0011,0xffff);	
     			
		}
		    
	}

/*----------------------------------------------------------------------------
 *        Task SMotor2
 *---------------------------------------------------------------------------*/
__task void SMotor2 (void) 
{
	
	M2Calculate_C0();                      /* Calculate C0 */
	M2Calculate_CnUp();                    /* Calculate Cn */
	
	Timer1_Int();                          /*initialize timer1 */
            
  while(1)
   	{	  
			  T1TCR         = 1;                             /* Timer1 Enable */
	      T1MR1         = M2delay_constant;
			  
			  M2step=M2step+1;
			
				if (M2step <=M2SegmentA1)
				{
				M2Calculate_CnUp();	
				}
				else if (M2step>M2SegmentA1 && M2step <=M2SegmentB1)
				{
				/*constant speed*/
				}
				else if (M2step >M2SegmentB1)
				{
				M2Calculate_CnDwn();
				}			
				
				if (M2step ==(M2total_steps-1))
				{
        T1MCR=0X00000004;                        /* stop on T1 */
				T1TCR         = 0;                       /* Timer0 disable */
				os_tsk_delete_self ();
				}

				os_evt_wait_and (0x0111,0xffff);			   
		}
		    
	}	
/*----------------------------------------------------------------------------
 *        Task SMotor3
 *---------------------------------------------------------------------------*/
__task void SMotor3 (void) 
{
	
	M3Calculate_C0();                      /* Calculate C0 */
	M3Calculate_CnUp();                    /* Calculate Cn */
	
	Timer2_Int();                          /*initialize timer1 */

  while(1)
   	{	  
			  T2TCR         = 1;                              /* Timer1 Enable */
	      T2MR2         = M3delay_constant;
			  
			  M3step=M3step+1;
			
				if (M3step <=M3SegmentA1)
				{
				M3Calculate_CnUp();	
				}
				else if (M3step>M3SegmentA1 && M3step <=M3SegmentB1)
				{
				/*constant speed*/
				}
				else if (M3step >M3SegmentB1)
				{
				M3Calculate_CnDwn();
				}			
				
				if (M3step ==(M3total_steps-1))
				{
        T2MCR= 0x00000100;                       /* stop on T2 */
				os_tsk_delete_self ();
				}
				
				os_evt_wait_and (0x1111,0xffff);			   
		}
		    
	}	

/*----------------------------------------------------------------------------
 *        Task 1 'init': Initialize
 *---------------------------------------------------------------------------*/
__task void init (void) {

  FIO2DIR  = 0x000000FF;                                   /* P2.0..7 defined as Outputs       */
  FIO2MASK = 0x00000000;
  FIO2PIN  = 0x00000000;
  PINSEL10 = 0; 

	serial_init ();
	motors_parameters_init();
	motors_calculations();
	
  t_SMotor1= os_tsk_create(SMotor1,0);                      /* start task        */
  t_SMotor2= os_tsk_create(SMotor2,0);                      /* start task        */
	t_SMotor3= os_tsk_create(SMotor3,0);                      /* start task        */
	
  os_tsk_delete_self ();
}

/*----------------------------------------------------------------------------
 *        Main: Initialize and start RTX Kernel
 *---------------------------------------------------------------------------*/
 int main (void) {
	 
  U32 volatile start;
                                                        
  for (start = 0; start < 1000000; start++) { ; }   /* Wait for debugger connection 0.3s*/
	
  os_sys_init (init);                  /* Initialize RTX and start init    */
	 
}

/*----------------------------------------------------------------------------
 * end of file
  *---------------------------------------------------------------------------*/


