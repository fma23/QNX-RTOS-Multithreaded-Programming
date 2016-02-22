/*----------------------------------------------------------------------------
 *      Name:   SpeedControl.C
 *      Author: Farid Mabrouk
 *      Purpose: AMT7 example program
 *----------------------------------------------------------------------------
 *      This code is part of the RealView Run-Time Library.
 *      Copyright (c) 2004-2011 KEIL - An ARM Company. All rights reserved.
 *---------------------------------------------------------------------------*/

#ifndef __motor_H__
#define __motor_H__


int unsigned angular_accel=        4;              //  in rad/sec^2
float  motor_step_angle=    0.0019625;      // 0.0019625*1000000000 need to normalize 

//int motor_step=             19625;    // need to normalize 
unsigned int frequency=     12000000;

#endif