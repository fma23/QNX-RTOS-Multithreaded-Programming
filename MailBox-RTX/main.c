/*----------------------------------------------------------------------------
 *      RL-ARM - RTX
 *----------------------------------------------------------------------------
 *      Name:    Main.C
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

extern void serial_init (void);
extern unsigned char sendchar (unsigned char character);
extern void SendInteger (int integer);
extern void printString(unsigned char *ptr);

extern void ADC_init(void);
extern void DAC_init(void);

OS_TID t_ADC;                      /* assigned task id: Send task (ADC) */
OS_TID t_rec_task;                 /* assigned task id: Receive task    */

/***********MAILBOX***********************************************************/

typedef struct {                                /* Message object structure           */
  unsigned int ADCVoltage;                      /* AD result of measured voltage      */
  unsigned int DACVoltage;                      /* DAC Voltage                        */ 
  unsigned char *message;                        /* transmit some message as a string */
} ADC_Meas;

os_mbx_declare (MsgBox, 16);                    /* Declare an RTX mailbox             */
ADC_Meas mpool[16*(2*sizeof(ADC_Meas))/4 + 3];  /* Reserve a memory for 16 messages   */

/*------------------------------------------------------------------------------------*/
/*                        ADC Task: send task                                         */
/*------------------------------------------------------------------------------------*/
	__task void ADC (void) {
		
		unsigned int AD_last;
    
		ADC_Meas *mptr;
			
	  os_mbx_init (MsgBox, sizeof(MsgBox));
    mptr = _alloc_box (mpool);                   /* Allocate a memory for the message   */
		os_dly_wait (5);                             /* Startup delay for MCB21xx           */

		while(1){
				AD_last = (AD0DR0 >> 6) & 0x3FF;         /* Read Conversion                       */
				DACR=  AD_last <<6 & 0x0000FFC0;         /* activate DAC with new ADC Value       */
			  AD0CR|=0x01000000;                       /* Start A/D Conversion                  */
			
				mptr->ADCVoltage = AD_last; 
				mptr->DACVoltage = AD_last<<2; 
			  mptr-> message= (unsigned char *)"This is a MailBox/Message Passing Demo for Keil RTX";	
			
				os_mbx_send (MsgBox, mptr, 0xffff);      /* Send the message to the mailbox     */
				os_dly_wait (100);
		}
					 
	}
/*-----------------------------------------------------------------------------*/
/*                         RECEIVE TASK                                        */
/*---------------------------------------------------------------------------- */
__task void rec_task (void) {
  
	ADC_Meas *rptr;
	
	for (;;) {
		
			os_mbx_wait (MsgBox, (void **)&rptr, 0xffff);    /* wait for the message                   */
			
			SendInteger(rptr->ADCVoltage);                   /* send the ADC value received to UART1   */
			sendchar(' ');
			
			DAC_init();
			DACR = rptr->DACVoltage <<6 & 0x0000FFC0;        /* send the ADC value received to the DAC */
		
			SendInteger(DACR);
			sendchar('\n');
		
		  printString(rptr->message);                      /* print the message received             */
		  sendchar('\n');
			
			_free_box (mpool, rptr);                         /* free memory allocated for message      */
 }

}

/*----------------------------------------------------------------------------
 *        Task 'init': Initialize
 *---------------------------------------------------------------------------*/
__task void init (void) {

  FIO2DIR  = 0x000000FF;                  /* P2.0..7 defined as Outputs       */
  FIO2MASK = 0x00000000;
  FIO2PIN  = 0x00000000;
  PINSEL10 = 0; 

	serial_init ();
	ADC_init();
	DAC_init();
	
	t_ADC       = os_tsk_create(ADC,1);
	t_rec_task  = os_tsk_create(rec_task,0);
	
  os_tsk_delete_self ();
}

/*----------------------------------------------------------------------------
 *        Main: Initialize and start RTX Kernel
 *---------------------------------------------------------------------------*/
 int main (void) {
	 
  U32 volatile start;
                                                        
 for (start = 0; start < 1000000; start++) { ; }   /* Wait for debugger connection 0.3s   */
	
	_init_box (mpool, sizeof(mpool),sizeof(int));    /* initialize the 'mpool' memory for   */
	
  os_sys_init (init);                              /* Initialize RTX and start init       */
	 
}

/*----------------------------------------------------------------------------
* end of file
*---------------------------------------------------------------------------*/


