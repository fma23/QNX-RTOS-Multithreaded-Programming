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

/*
 param step   Number of steps to move (pos - CW, neg - CCW).
 param accel  Accelration to use, in 0.01*rad/sec^2.
 param decel  Decelration to use, in 0.01*rad/sec^2.
 param speed  Max speed, in 0.01*rad/sec.

 Number of steps before we must start deceleration (if accel does not hit max speed).
 ccel_lim;
  
 Find out after how many steps does the speed hit the max speed limit.
 max_s_lim = speed^2 / (2*alpha*accel)
 
 Find out after how many steps we must start deceleration.
 n1 = (n1+n2)decel / (accel + decel)
 accel_lim = ((long)step*decel) / (accel+decel);    
 */

extern unsigned int M1SegmentA1; 
extern unsigned int M1SegmentB1;
extern unsigned int M1total_steps;
extern unsigned int M1MaxSpeed;                      /* in rad/sec     */
extern float M1StepAngle;                     /* in rad         */
extern unsigned int M1Acceleration;
extern unsigned int M1Decceleration;

extern unsigned int M2SegmentA1; 
extern unsigned int M2SegmentB1;
extern unsigned int M2total_steps;
extern unsigned int M2MaxSpeed;                      /* in rad/sec     */
extern float M2StepAngle;                            /* in rad         */
extern unsigned int M2Acceleration;
extern unsigned int M2Decceleration;

extern unsigned int M3SegmentA1; 
extern unsigned int M3SegmentB1;
extern unsigned int M3total_steps;
extern unsigned int M3MaxSpeed;                      /* in rad/sec     */
extern float M3StepAngle;                            /* in rad         */
extern unsigned int M3Acceleration;
extern unsigned int M3Decceleration;
extern unsigned int Frequency;

extern char sendchar (char character);
extern void SendInteger (int integer);

void motors_parameters_init(void){
	
M1total_steps = 4200;
M1StepAngle = 0.0019625;          /* in rad        */
M1MaxSpeed = 4;  
M1Acceleration = 2;               /* in rad/sec^2  */
M1Decceleration = 2;              /* in rad/sec^2  */
	
M2total_steps = 9000;
M2StepAngle = 0.0019625;          /* in rad        */
M2MaxSpeed = 5;  
M2Acceleration = 2;               /* in rad/sec^2  */
M2Decceleration = 2;              /* in rad/sec^2  */
	
M3total_steps = 6200;
M3StepAngle = 0.0019625;          /* in rad        */
M3MaxSpeed = 7;  
M3Acceleration = 3;               /* in rad/sec^2  */
M3Decceleration = 3;              /* in rad/sec^2  */
	
Frequency=     12000000;
	
}
void motors_calculations(void){

float temp1;
float temp2;

 /*Find out after how many steps does the speed hit the max speed limit.
 max_s_lim = speed^2 / (2*alpha*accel) */

/**MOTOR1 **/
temp1= M1MaxSpeed*M1MaxSpeed;
temp2=2*M1StepAngle*M1Acceleration;
M1SegmentA1=ceil(temp1/temp2);                 /* Max_S_lim  */

/**MOTOR2 **/
temp1= M2MaxSpeed*M2MaxSpeed;
temp2=2*M2StepAngle*M2Acceleration;
M2SegmentA1=ceil(temp1/temp2);                 /* Max_S_lim  */

/**MOTOR3 **/
temp1= M3MaxSpeed*M3MaxSpeed;
temp2=2*M3StepAngle*M3Acceleration;
M3SegmentA1=ceil(temp1/temp2);                 /* Max_S_lim  */
	
/*Find out after how many steps we must start deceleration.
 n1 = (n1+n2)decel / (accel + decel)
 accel_lim = ((long)step*decel) / (accel+decel);    
 */

/**MOTOR1 **/
temp1= M1total_steps*M1Decceleration;
temp2= M1Acceleration+M1Decceleration;
M1SegmentB1= ceil(temp1/temp2);               /* Accel_lim   */
 
/**MOTOR2 **/
temp1= M2total_steps*M2Decceleration;
temp2= M2Acceleration+M2Decceleration;
M2SegmentB1= ceil(temp1/temp2);               /* Accel_lim   */

/**MOTOR3 **/
temp1= M3total_steps*M3Decceleration;
temp2= M3Acceleration+M3Decceleration;
M3SegmentB1= ceil(temp1/temp2);               /* Accel_lim               */

/**MOTOR1 **/
if(M1SegmentA1 < M1SegmentB1) {               /* Max_S_lim <  Accel_lim   */

		 temp1= M1Acceleration/M1Decceleration;
		 M1SegmentB1= M1SegmentA1*temp1;
		 M1SegmentB1=M1total_steps-M1SegmentB1;
}
else {
	    M1SegmentB1=M1total_steps-M1SegmentB1;
		  M1SegmentA1=M1SegmentB1;
}

/**MOTOR2 **/	
if(M2SegmentA1 < M2SegmentB1) {               /* Max_S_lim <  Accel_lim   */
	
		 temp1= M2Acceleration/M2Decceleration;
		 M2SegmentB1= M2SegmentA1*temp1;
		 M2SegmentB1=M2total_steps-M2SegmentB1;
}
else{	
	    M2SegmentB1=M2total_steps-M2SegmentB1;
		  M2SegmentA1=M2SegmentB1;
}

/**MOTOR3 **/
if(M3SegmentA1 < M3SegmentB1) {               /* Max_S_lim <  Accel_lim   */
	
		 temp1= M3Acceleration/M3Decceleration;
		 M3SegmentB1= M3SegmentA1*temp1;
		 M3SegmentB1=M3total_steps-M3SegmentB1;
}
else {
	
	    M3SegmentB1=M3total_steps-M3SegmentB1;
		  M3SegmentA1=M3SegmentB1;
}
 /* For debugging purposes */
	 SendInteger(M1SegmentA1);
   sendchar (' ');
	 SendInteger(M1SegmentB1);
   sendchar (' ');

   SendInteger(M2SegmentA1);
   sendchar (' ');
	 SendInteger(M2SegmentB1);
   sendchar (' ');

   SendInteger(M3SegmentA1);
   sendchar (' ');
	 SendInteger(M3SegmentB1);
   sendchar (' ');
}