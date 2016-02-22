#include <stdio.h>
#include <stdlib.h>
#include <process.h>
#include <sched.h>
#include <unistd.h>
#include <sys/neutrino.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <sys/siginfo.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <time.h>
#include <math.h>

#include "SCORBOT.H"
#include "PPORT.H"
#include "CONST.H"
#include "GLOB.H"


void 	writeMsg (int);
void 	readMsg(int,int);
void*	timer_thread(void*notused);
void* 	timer2_thread(void*notused);
void* 	GUI_thread(void*notused);
void* 	auto_thread(void*notused);
int		homing();
void	auto_mode();
void*	SIM_thread(void*notused);
void* 	Handlerloop_thread(void*notused);
void 	PulseHandler(int);
void 	mainLoop();
char 	kb_getc(void);
void 	printMenu();
void* 	keyboard_input(void*notused);
void*	blockID();
void	pattern();
int 	findBlock(block_t block);

main (void)
{
	pid_t	pid_gui;
	pid_t	pid_sim;
	pid_t	pid_pport;
	
	int		simChid;

	
	int		kill_gui;
	int		kill_sim;
	int		kill_pport;
	
	int		s_return;	
	
	//---thread paramters--------------//
	pthread_attr_t attr;
	pthread_t timerThreadID;
	pthread_t keyboardThreadID;
	pthread_t blockIDThreadID;
	
	do
	{
	printf("\n=========================================================\n");
	printf("Please Selet the Operating Mode:\n");
	printf("1: Basic Orientation Mode for Block: 0-10).\n");
	printf("2: Rotated Orientation Mode for Block:0-6). (alaph version)\n");
	printf("===========================================================\n");
	flushall();
	scanf ("%c",&programMode);
	printf("You have selected: Mode %c.\n\n",programMode);
	flushall();
	if((programMode != '1' )&&(programMode !='2'))
	{
		printf("Invalid Selection, Please Enter!\n");
		flushall();
	}
	}
	while(programMode != '1'&&programMode !='2');
	
	


// ----------------Share Memory----------------------------------------
	shMem=shm_open("shared_memory", OFLAGS, 0777);
	if (shMem == -1)
	{
		printf("shared_memory failed to open...\n");
		flushall();
	}
	else
	{
		if (ftruncate (shMem,SIZE) == -1)
		{
			printf ("Failed to set size for -- shmem -- \n");
			flushall();
		}
		else
		{
			//mapping a shared memory location
			memLocation = mmap (0,  SIZE, PROT, MFLAGS, shMem, 0);
			if (memLocation == MAP_FAILED)
			{
				printf (" Failed to map to shared memory...\n");
				flushall();
			}
		}	
	}
// ---------------Semorphore-------------------------------------
	// semorphore for shared memory
	sem = sem_open ("shared_sem", O_CREAT, S_IRWXG | S_IRWXO | S_IRWXU, 0);
	if (sem == (sem_t *)(-1)) 
	{
	   	printf ("User: Memory Semaphore failed to open....\n");
	  	flushall();
	}
	else
	{
		sem_post(sem);
	}
// -----------------------channel creation---------------------------
    // Create a channels for the simulator and Gui
    // The ChannelCreate function returns the channel ID
	ChannelCreate(0);
	ChannelCreate(0);
	//ChannelCreate(0);//for pport
	
	simChid = 1;
	sleep(1);
	
	// Spawing a process for the GUI and Simulator
	pid_gui = spawnl(P_NOWAIT, "/usr/local/bin/gui_g", "gui_g", NULL);
	pid_sim = spawnl(P_NOWAIT, "/usr/local/bin/newGUIPport_g", "sim", NULL);
	pid_pport = spawnl(P_NOWAIT, "/usr/local/bin/testPport_g", "pport",NULL);
	
	sleep(1);
	//The Gui process automatically connect to the channel
	//Thus we only need to attach the simulator process to the created channel
	coidsim = ConnectAttach(0,pid_sim,simChid,0,0);
	// Display error message if connection failed
	if (coidsim == -1)
	{
		printf("coidsim error\n");
		flushall();
		exit(EXIT_FAILURE);
	}
	coidpport = ConnectAttach(0,pid_pport,simChid,0,0);
	// Display error message if connection failed
	if (coidpport == -1)
	{
		printf("coidpport error\n");
		flushall();
		exit(EXIT_FAILURE);
	}

// --------------------------timer code----------------------------------
	// Create a channel for sending a pulse to myself when timer expires
	timerChid = ChannelCreate(_NTO_CHF_UNBLOCK);
	if(timerChid == -1)
	{
		printf("timer Channel create failed\n");		
	    	flushall();
	}
	timerCoid = ConnectAttach ( 0, getpid ( ), timerChid, 0, 0);
   	if(timerCoid == -1 ) 
   	{
       	printf ("Channel attach failed!\n");
		flushall();
		perror ( NULL ); 
		exit ( EXIT_FAILURE);
   	}
	//	Set up pulse event for delivery when the first timer expires; pulse code = 8, pulse value  = 0;
	SIGEV_PULSE_INIT (&timerEvent, timerCoid, SIGEV_PULSE_PRIO_INHERIT, 8, 0);
	//	Create Timer
	if (timer_create (CLOCK_REALTIME, &timerEvent, &timerid) == -1) 
   	{
   		printf ( "Failed to create a timer for pulse delivery\n");
		flushall();
   		perror (NULL);
   		exit ( EXIT_FAILURE);
   	}
	// Setup one time timer for 2 second
	timer.it_value.tv_sec = 2;
	timer.it_value.tv_nsec = 0;
	timer.it_interval.tv_sec = 0;
	timer.it_interval.tv_nsec = 0;
// --------------------------timer2 code----------------------------------
	// Create a channel for sending a pulse to myself when timer expires
	timerChid2 = ChannelCreate(_NTO_CHF_UNBLOCK);
	if(timerChid2 == -1)
	{
		printf("timer Channel create failed\n");		
	    	flushall();
	}
	timerCoid2 = ConnectAttach ( 0, getpid ( ), timerChid2, 0, 0);
   	if(timerCoid2 == -1 ) 
   	{
       	printf ("Channel attach failed!\n");
		flushall();
		perror ( NULL ); 
		exit ( EXIT_FAILURE);
   	}
	// Set up pulse event for delivery when the first timer expires; pulse code = 8, pulse value  = 0;
	SIGEV_PULSE_INIT (&timerEvent2, timerCoid2, SIGEV_PULSE_PRIO_INHERIT, 8, 0);
	// Create Timer
	if (timer_create (CLOCK_REALTIME, &timerEvent2, &timerid2) == -1) 
   	{
   		printf ( "Failed to create a timer for pulse delivery\n");
		flushall();
   		perror (NULL);
   		exit ( EXIT_FAILURE);
   	}
//-------------------timer monitor thread--------------------
	pthread_attr_init(&attr);
	pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
	pthread_create(&timerThreadID,&attr,timer_thread,NULL);
	if(timerThreadID == -1)
	{
		printf("Fail to create timer thread!");
		flushall();
	}
	else
	{
		printf("The timer thread ID is %i \n",timerThreadID);
		flushall();
	}
	pthread_create(&timerThreadID,&attr,timer2_thread,NULL);
	if(timerThreadID == -1)
	{
		printf("Fail to create timer thread2!");
		flushall();
	}
	else
	{
		printf("The timer thread2 ID is %i \n",timerThreadID);
		flushall();
	}
//
//----start the block Identification thread-----------------
//	pFile = fopen("Blocks.txt","w+");
	pthread_create(&blockIDThreadID,&attr,blockID,NULL);
		if(blockIDThreadID == -1)
	{
		printf("Fail to create block indeptification thread!");
		flushall();
	}
	else
	{
		printf("The BlockID's thread ID is %i \n",blockIDThreadID);
		flushall();
	}
	delay(10);
//---------------------keyboard thread------------------------
	pthread_create(&keyboardThreadID,&attr,keyboard_input,NULL);
		if(keyboardThreadID == -1)
	{
		printf("Fail to create keyboard input!");
		flushall();
	}
	else
	{
		printf("The keyboard_input ID is %i \n",timerThreadID);
		flushall();
	}
	delay(10);
//----------------GUI Monitor Thread---------------------
	pthread_create(NULL,&attr,GUI_thread,NULL);
	delay(10);
// --------------------SIM pulse Handler Loop Thread----------
	pthread_create(NULL,&attr,Handlerloop_thread,NULL);
	delay(10);
// --------------------SIM Monitor Thread---------------------
	pthread_create(NULL,&attr,SIM_thread,NULL);
	delay(10);
// --------------------Auto mode thread-----------------------
	pthread_create(NULL,&attr,auto_thread,NULL);
	delay(10);
// --------------------others---------------------------------
	// call mainLoop()
	mainLoop();
	// sleep for 10sec
	printf("Sleep the program for 10 seconds\n");
	flushall;
	sleep(10);
	// start release system resourse
	printf("Clean up and release system resourse\n");
	flushall;
	// Kill the existing processes
	kill_gui = kill(pid_gui, 0);
	kill_sim = kill(pid_sim, 0);
	kill_pport = kill(pid_pport, 0);
    if (kill_gui == -1)
	{
		printf("GUI Kill failed\n");
		flushall();
	}
	if (kill_sim == -1)
	{
		printf("SIM Kill failed\n");
		flushall();
	} 
	if (kill_pport == -1)
	{
		printf("PPort Kill failed\n");
		flushall();
	}
	// Close and unlink Semorphore
	sem_close(sem);
	s_return = sem_unlink("/dev/sem/home/quad6/workspace/Project_S1/shared_sem");
	// Display error messae if semorphonre unlink is failed
	if (s_return == -1)
	{
		printf("a: %s\n", strerror( errno ));
		flushall();
	}
	// Close, unmap, and unlink shared memory
	shm_close(shMem);
	munmap(shmLocation,SIZE);
	shm_unlink("shared_memory");
	// Detach the connections and destroy the channels
	ConnectDetach(coidGui);
    ConnectDetach(coidsim);
    ConnectDetach(coidpport);
    ChannelDestroy(guiChid);
    ChannelDestroy(simChid);
    
//    fclose(pFile);
}	

// --------------------Function/Thread/Handler code--------------------
// ----------------------------WriteMsg--------------------------------
void writeMsg(int command)
{
	// Reset output set byte when command is not conveyor OFF/ON
	if ((command != CONVEYOROFF) && (command != CONVEYORON))
	{
		// Reset hi output set byte
		highOutput.hiOutputSetByte = 0;
		// Reset low output set byte except conveyor_ON_nOFF
		if (lowOutput.conveyor_ON_nOFF == OFF)
			lowOutput.lowOutputSetByte = 0;
		else
			lowOutput.lowOutputSetByte = 8;
	}
	// msgType => write
	smsg.msgType = WRITE;
	switch (command)
	{
		// elbow joint control
		case (ELBOWUP):
		case (ELBOWDW):
			highOutput.elbow_ON_nOFF = ON;
			if (command == ELBOWUP)
				highOutput.elbowDir_UP_nDOWN = elbow_UP;
			else 
				highOutput.elbowDir_UP_nDOWN = elbow_DOWN;
			break;
		// gripper movement control
		case (GRIPPEROP):
		case (GRIPPERCLOSE):
			highOutput.gripper_ON_nOFF = ON;
			if (command == GRIPPEROP)
				highOutput.gripperDir_CLOSE_nOPEN = gripper_OPEN;
			else
				highOutput.gripperDir_CLOSE_nOPEN = gripper_CLOSE; 
			break;
		// pitch movement control
		case PITCHUP :highOutput.rollPitch4Bits = pitch_UP; break;
		case PITCHDW :highOutput.rollPitch4Bits = pitch_DOWN; break;
		// roll movement control
		case ROLLCW  :highOutput.rollPitch4Bits = roll_CW; break;
		case ROLLCCW :highOutput.rollPitch4Bits = roll_CCW; break;
		// base movement control
		case (BASECW):

		case (BASECCW):
			lowOutput.base_ON_nOFF = ON;
			if (command == BASECW)
				lowOutput.baseDir_RIGHT_nLEFT = baseDir_RIGHT;
			else
				lowOutput.baseDir_RIGHT_nLEFT = baseDir_LEFT;
			break;
		// shoulder joint control
		case (SHOULDERUP):
		case (SHOULDERDW):
			lowOutput.shoulder_ON_nOFF = ON;
			if (command == SHOULDERUP)
				lowOutput.shoulderDir_DOWN_nUP = shoulderDir_UP;
			else
				lowOutput.shoulderDir_DOWN_nUP = shoulderDir_DOWN;
			break;
		// conveyor belt control
		case CONVEYOROFF: lowOutput.conveyor_ON_nOFF = OFF; break;
		case CONVEYORON : lowOutput.conveyor_ON_nOFF = ON; 	break;
		// right unloader on/off
		case UNLOADRIGHT:lowOutput.unloaderRight_ON_nOFF = ON; break;
		// left unloader on/off
		case UNLOADLEFT:lowOutput.unloaderLeft_ON_nOFF = ON; break;
		case ESTOP: lowOutput.conveyor_ON_nOFF = OFF;
		case COMMEND_RESET :
			highOutput.elbow_ON_nOFF = OFF;
			highOutput.gripper_ON_nOFF = OFF;
			lowOutput.base_ON_nOFF = OFF;
			lowOutput.shoulder_ON_nOFF = OFF;
			lowOutput.unloaderRight_ON_nOFF = OFF;
			lowOutput.unloaderLeft_ON_nOFF = OFF;
			break;
		default:
			break;
	}
	// update smsg
	smsg.rwdataMsg.upper_data = highOutput.hiOutputSetByte;
	smsg.rwdataMsg.lower_data = lowOutput.lowOutputSetByte;
	if(MsgSend(coidsim,&smsg,sizeof(smsg),NULL,NULL) == -1)
	{
		printf("Sim message send failed\n");
		flushall();
	}
	if(command == CONVEYOROFF||command==CONVEYORON||command==ESTOP||command==COMMEND_RESET)
	{
		if(MsgSend(coidpport,&smsg,sizeof(smsg),NULL,NULL) == -1)
		{
			printf("Pport message send failed\n");
			flushall();
		}
	}
}
// -------------------------------ReadMsg-----------------------------------------
void readMsg(int system, int sensor)
{
	hiInputSet simUpperByte;
	hiInputSet pportUpperByte;
	// msgType => write
	smsg.msgType = READ;
	// command the simulator to return system status
	MsgSend(coidsim,&smsg,sizeof(smsg),&rmsg,sizeof(rmsg));
	// extract system status data
	simUpperByte.hiInputSetByte = rmsg.rwdataMsg.upper_data;
	
	// Send the status request to the PPORT, only need the lower byte data.
	MsgSend(coidpport,&smsg,sizeof(smsg),&rmsg,sizeof(rmsg));
	readLowerByte.lowInputSetByte = rmsg.rwdataMsg.lower_data;
	pportUpperByte.hiInputSetByte = rmsg.rwdataMsg.upper_data;
	// combine the sim and pport pulses
	readUpperByte.loader_nEMPTY     = pportUpperByte.loader_nEMPTY;
	readUpperByte.unloader_nEMPTY   = pportUpperByte.unloader_nEMPTY;
	readUpperByte.baseLimit_nBL     = simUpperByte.baseLimit_nBL;
	readUpperByte.shoulderLimit_nBL = simUpperByte.shoulderLimit_nBL;
	readUpperByte.elbowLimit_nBL    = simUpperByte.elbowLimit_nBL;
	readUpperByte.pitchLimit_nBL    = simUpperByte.pitchLimit_nBL;
	readUpperByte.rollLimit_nBL     = simUpperByte.rollLimit_nBL;
	readUpperByte.hiIpReserved7     = simUpperByte.hiIpReserved7;
	 // Check whether system status has changed and whether user wants to display it
	if ((system == 1) && (memData.upperByte != readUpperByte.hiInputSetByte))
	{
		// display interpretation
		if (readUpperByte.baseLimit_nBL == 1)
			printf("base working outside limit!\n");
		if (readUpperByte.shoulderLimit_nBL == 0)
			printf("shoudler outside limit!\n");
		if (readUpperByte.elbowLimit_nBL == 0)
			printf("elbow outside its limit!\n");
		if (readUpperByte.pitchLimit_nBL == 0)
			printf("pitch outside its limit!\n");
		if (readUpperByte.rollLimit_nBL == 0)
			printf("roll outside its limit!\n");
		if (readUpperByte.unloader_nEMPTY == 0)
			printf("Unloader detect a block.\n");
		if (readUpperByte.loader_nEMPTY == 1)
			printf("loader outside its limit!\n");
//		printf("\n");
		flushall();
	}
	// Check whether user wants to see the sensor states and whether sensor states has changed
	if ((sensor == 1) && ((system == 1))) //&& (memData.lowerByte != readLowerByte.lowInputSetByte)))
	{
		printf(" A2: %i\n B1: %i\n B4: %i\n C3: %i\n C2: %i\n C1: %i\n\n",readLowerByte.sensor_nA2,readLowerByte.sensor_nB1,readLowerByte.sensor_nB4,readLowerByte.sensor_nC3,readLowerByte.sensor_nC2,readLowerByte.sensor_nC1);
		flushall();
	}
}

//------------------------------Timer Thread--------------------------------------
void* timer_thread(void*notused)
{
	int time_count;
	int rcvid_timer;
	while(1)
	{	
		rcvid_timer = MsgReceivePulse(timerChid, &timerPulse, sizeof(timerPulse), NULL);
		if (rcvid_timer==-1)
		{
			printf("Error in timer pulse!\n");
			flushall();
		}
		else
		{writeMsg(COMMEND_RESET);}
		delay(50);
			
	}
}
// ------------------------------Timer2 Thread--------------------------------------
void* timer2_thread(void*notused)
{
	int rcvid_timer;
	while(1)
	{	
		rcvid_timer = MsgReceivePulse(timerChid2, &timerPulse, sizeof(timerPulse), NULL);
		if (rcvid_timer==-1)
		{
			printf("Error in timer2 pulse!\n");
			flushall();
		}
		else
			pulse_count++;
	}
}
// ------------------------------GUI Monitor Thread-------------------------------
void* GUI_thread(void*notused)
{
	int guiPulseVal;

	highOutput.hiOutputSetByte = 0;
	lowOutput.lowOutputSetByte = 0;
	while(1)
	{
		// Receive message from GUI
		MsgReceivePulse (2,&guiPulse,sizeof(guiPulse),NULL);
		// initialize guiPulseVal
		// extract the pulse value
		if (guiPulse.value.sival_int != COMMEND_RESET)
		{
			guiPulseVal = 0; 
			guiPulseVal = guiPulse.value.sival_int;
			last_input  = guiPulseVal;
		}
		// print GUI pulse
//		printf("GUI: Pulse: %i \n", guiPulseVal);
//		printf("Temp: Temp = %i \n", temp_guiPulse);
		// Quit loop if guiPulseVal == GUIQUIT
		if (guiPulseVal == GUIQUIT) {killFlg = 1; break;}
		// Command simulator
		writeMsg(guiPulseVal);
		if ((guiPulseVal == BASECW) 	|| (guiPulseVal == BASECCW)  	||
		    (guiPulseVal == SHOULDERUP) || (guiPulseVal == SHOULDERDW)	||
			(guiPulseVal == ELBOWUP)	|| (guiPulseVal == ELBOWDW)		||
			(guiPulseVal == PITCHUP) 	|| (guiPulseVal == PITCHDW)		||
			(guiPulseVal == UNLOADLEFT) || (guiPulseVal == UNLOADRIGHT)	||
			(guiPulseVal == GRIPPEROP)  || (guiPulseVal == GRIPPERCLOSE))
		{
			timer_settime(timerid, 0, &timer,NULL);
		}
	}
}
// ------------------------------Auto Thread-------------------------------------
void* auto_thread(void*notused)
{
	int homed;
	homed = 0;
	while(1)
	{
		//printf("home: %i\n",homed);
		if ((last_input == BASECW)   	|| (last_input == BASECCW)  	||
		    (last_input == SHOULDERUP)  || (last_input == SHOULDERDW)	||
			(last_input == ELBOWUP)  	|| (last_input == ELBOWDW)		||
			(last_input == PITCHUP) 	|| (last_input == PITCHDW)		||
			(last_input == GRIPPEROP)   || (last_input == GRIPPERCLOSE) ||
			(last_input == ROLLCW)      || (last_input == ROLLCCW))
			homed = 0;
		if (memData.autoModeOn == ON)
		{
			if (last_input == HOMEROBOT)
			{
				printf("homing!\n");
				if (homed != 1)
				{
					homed = homing();
					last_input = MANUALON;
					writeMsg(last_input);
				}
				printf("home done\n");
				flushall();
				last_input = MANUALON;
				writeMsg(last_input);
			}
			if (last_input == MANUALOFF)
			{
				if (readLowerByte.lowIpReserved1 == 1)
				{
					printf("No block in the loader\n");
					flushall();
				}
				else
				{
					if (homed != 1)
						homing();
					homed = 0;
					auto_mode();
				}
				last_input = MANUALON;
				writeMsg(last_input);
				printf("auto done\n");
				flushall();
			}
		}
		delay(50);
	}
}
// -----------------------------home robot function-------------------------
int homing()
{
	int step;
	int instr;
	int rcvid_timer;
	step = 1;
	while (memData.autoModeOn == ON)
	{
		switch(step)
		{
			case 1 : // done
				instr = BASECCW;
				break;
			case 2 :
				instr = BASECW;
				timer2.it_value.tv_sec = 0;
				timer2.it_value.tv_nsec = 800000000;
				break;
			case 3 : // done
				instr = GRIPPEROP;
				timer2.it_value.tv_sec = 1;
				timer2.it_value.tv_nsec = 0;
				break;
			case 4 : // done
				instr = SHOULDERUP;
				break;
			case 5 :
				instr = SHOULDERDW;
				timer2.it_value.tv_sec = 0;
				timer2.it_value.tv_nsec = 600000000;
				break;
			case 6 : // done
				instr = ELBOWUP;
				break;
			case 7 :
				instr = ELBOWDW;
				timer2.it_value.tv_sec = 0;
				timer2.it_value.tv_nsec = 400000000;
				break;
			case 8 : // done
				instr = PITCHUP;
				break;
			case 9 :
				instr = PITCHDW;
				timer2.it_value.tv_sec = 0;
				timer2.it_value.tv_nsec = 450000000;
				break;
			case 10 : // done
				instr = ROLLCW;
				break;
			case 11 : // done
				instr = ROLLCCW;
				timer2.it_value.tv_sec = 0;
				timer2.it_value.tv_nsec = 1000;
				break;
		}
		if ((step == 1) || (step == 4) || (step == 6) || (step == 8) || (step == 10))
		{
			while ((memData.autoModeOn == ON) && (((simPulseVal&64) + (simPulseVal&32) + (simPulseVal&16) + (simPulseVal&8) + (simPulseVal&4)) == 120))
			{
				last_input = instr;
				writeMsg(last_input);
				delay(50);
			}
			delay(100);
			while ((memData.autoModeOn == ON) && (((simPulseVal&64) + (simPulseVal&32) + (simPulseVal&16) + (simPulseVal&8) + (simPulseVal&4)) != 120))
				delay(150);
		}
		else
		{
			pulse_count = 0;
			timer_settime(timerid2, 0, &timer2,NULL);
			while ((pulse_count == 0) && (memData.autoModeOn == ON))
			{
				writeMsg(instr);
				delay(50);
			}
			writeMsg(COMMEND_RESET);
		}
		if (step != 11)
			step++;
		else
		{
			return 1; 
		}
	}
	return 0;
}
// -----------------------------auto mode function-------------------------
void auto_mode()
{
	int step;
	int instr;
	int rcvid_timer;
	step = 1;
	while (memData.autoModeOn == ON)
	{
		switch(step)
		{
			case 1  : 
				instr = SHOULDERDW;
				timer2.it_value.tv_sec = 2;
				timer2.it_value.tv_nsec = 600000000;
				break;
			case 2  : 
				instr = GRIPPERCLOSE;
				timer2.it_value.tv_sec = 1;
				timer2.it_value.tv_nsec = 0;
				break;
			case 3  : 
				instr = SHOULDERUP;
				timer2.it_value.tv_sec = 2;
				timer2.it_value.tv_nsec = 600000000;
				break;
			case 4  : 
				instr = BASECW;
				timer2.it_value.tv_sec = 3;
				timer2.it_value.tv_nsec = 100000000;
				break;
			case 5  :
				instr = SHOULDERDW;
				timer2.it_value.tv_sec = 2;
				timer2.it_value.tv_nsec = 600000000;
				break;
			case 6  :
				instr = ELBOWDW;
				timer2.it_value.tv_sec = 2;
				timer2.it_value.tv_nsec = 100000000;
				break;
			case 7  :
				instr = PITCHDW;
				timer2.it_value.tv_sec = 1;
				timer2.it_value.tv_nsec = 400000000;
				break;
			case 8  :
				instr = GRIPPEROP;
				timer2.it_value.tv_sec = 1;
				timer2.it_value.tv_nsec = 100000000;
				break;
			case 9  :
				instr = PITCHUP;
				timer2.it_value.tv_sec = 1;
				timer2.it_value.tv_nsec = 400000000;
				break;
			case 10 :
				instr = ELBOWUP;
				timer2.it_value.tv_sec = 2;
				timer2.it_value.tv_nsec = 100000000;
				break;
			case 11 :
				instr = SHOULDERUP;
				timer2.it_value.tv_sec = 2;
				timer2.it_value.tv_nsec = 600000000;
				break;
			case 12 :
				instr = BASECCW;
				timer2.it_value.tv_sec = 3;
				timer2.it_value.tv_nsec = 100000000;
				break;
		}	
		pulse_count = 0;
		timer_settime(timerid2, 0, &timer2,NULL);
		while ((pulse_count == 0) && (memData.autoModeOn == ON))
		{
			writeMsg(instr);
			delay(50);
		}
		writeMsg(COMMEND_RESET);
		if (step != 12)
			step++;
		else
			step = 1; 
	}
}
// ------------------------------Simulator Monitor Thread------------------------
void* SIM_thread(void*notused)
{	
	struct _pulse intrPulse;
	while(killFlg == 0)
	{
		// Receive message from SIM
		MsgReceivePulse (1,&intrPulse,sizeof(intrPulse),NULL);
		temp_last_input = last_input;		
		simPulse = intrPulse;
		// Extract simulator pulse value
		simPulseVal = simPulse.value.sival_int;
		delay(100);
	}
}
// ------------------------------Handler Loop--------------------------------
void* Handlerloop_thread(void*notused)
{
	// Initialize simulator pulse value
	simPulseVal = 120;
	while(killFlg == 0)
	{
		// Keep calling PulseHandler until all joint is within limit
		while(((simPulseVal&64) + (simPulseVal&32) + (simPulseVal&16) + (simPulseVal&8) + (simPulseVal&4)) != 120)
		{
			//printf("\nin while\n");
			PulseHandler(simPulseVal);
			if (((simPulseVal&64) + (simPulseVal&32) + (simPulseVal&16) + (simPulseVal&8) + (simPulseVal&4)) == 120)
			{
				writeMsg(COMMEND_RESET);
				delay(50);
				//printf("SIM: Pulse: %i %i %i %i %i \n\n", (simPulseVal&64)/64, (simPulseVal&32)/32, (simPulseVal&16)/16, (simPulseVal&8)/8, (simPulseVal&4)/4);
			}
		}
		delay(50);
	}
}
// ------------------------------Pulse Handler----------------------------------
void PulseHandler(int simPV_L)
{
	// move the links back into limit range
	if ((simPV_L&64 )/64 == 0 )
	{
		if (temp_last_input == ROLLCW)	writeMsg(ROLLCCW);
		else							writeMsg(ROLLCW); 
	}
	else if ((simPV_L&32)/32 == 0)
	{
		if (temp_last_input == PITCHUP)	writeMsg(PITCHDW);
		else							writeMsg(PITCHUP); 
	}
	else if ((simPV_L&16)/16 == 0)
	{
		if (temp_last_input == ELBOWUP)	writeMsg(ELBOWDW);
		else							writeMsg(ELBOWUP); 
	}
	else if ((simPV_L&8)/8 == 0)
	{
		if (temp_last_input == SHOULDERUP)	writeMsg(SHOULDERDW);
		else								writeMsg(SHOULDERUP); 
	}
	else if ((simPV_L&4)/4 == 1)
	{
		if (temp_last_input == BASECW)	writeMsg(BASECCW);
		else							writeMsg(BASECW); 
	}
}
// ------------------------------Main Loop---------------------------------------
void mainLoop()
{
	writeMsg(ESTOP);
	while(killFlg == 0)
	{	
		readMsg(1,0);
		// shared memory access
			// update system states in shared memory
			if (last_input == CONVEYORON)
				memData.conveyorOn = ON;
			else if ((last_input == CONVEYOROFF) || (last_input == ESTOP))
				memData.conveyorOn = OFF;
			if ((last_input == MANUALON) || (last_input == ESTOP))
				memData.autoModeOn = OFF;
			else if (last_input == MANUALOFF || last_input == HOMEROBOT)
				memData.autoModeOn = ON;	
			memData.upperByte = readUpperByte.hiInputSetByte;
			memData.lowerByte = readLowerByte.lowInputSetByte;
			memData.blkType = blockNumG;
		sem_wait(sem);
			*memLocation = memData;
		sem_post(sem);
		//  request status from GUIport and write to shared memory for every 50 ms
		delay(50);
	}
}

//-------------------------------keyboard code----------------------------------
/* kb_getc(void) takes in the keyboard input */
char kb_getc(void){
	unsigned char ch;
	int flags;
	int retval;
	ssize_t size;
	//----------------
	flags = fcntl( 0, F_GETFL );
    flags |= O_NONBLOCK;
    retval = fcntl( 0, F_SETFL, flags );
	size = read(0, &ch, 1);
	//----------------
	if(size == -1)
	{
		return 0;
	} else {
		return ch;
	}
}
//-------------------------------PrintMenu-----------------------------------------------
void printMenu()
{
	sleep(1);
	printf("\n");
	printf("                Base cw/ccw (B/b)\n");
	printf("                Shoulder up/down (S/s)\n");
	printf("                Elbow up/down (E/e)\n");
	printf("                Pitch up/down (P/p)\n");
	printf("                Roll cw/ccw (R/r)\n");
	printf("                Gripper open/close (G/g)\n");
	printf("                Conveyor belt on/off (C/c)\n");
	printf("                Unload solenoid Left on (L)\n");
	printf("                Unload solenoid Right on (l)\n");
	printf("                Home (H/h)\n");
	printf("                Automode on/off (A/a)\n");
	printf("                Estop ('spacebar')\n");
	printf("                Display sensor readings (D/d)\n");
	printf("                Quit(Q/q)\n");
	printf("                Print Menu (M/m)\n");
	printf("                Please enter selection:\n\n");
	flushall();
}
//-------------------------------Keyboard_input------------------------------------------
void* keyboard_input(void* notuse)
{
	char kbInput = 0; //Stores keyboard input
	char out_msg[100];
	int  instr;
	printMenu();
	while(killFlg == 0)
	{
		instr = 100;
		strcpy(out_msg,"o");
		kbInput = kb_getc();
		if (memData.autoModeOn == OFF || kbInput == ' ' || kbInput == 'a' || 
			kbInput == 'c' || kbInput == 'C' || kbInput == 'L' || kbInput == 'l')
		switch (kbInput)
		{
			case 'q':
			case 'Q':
				strcpy(out_msg, "Program Exiting... \n");
				instr = GUIQUIT;
				killFlg = 1;
				break;
			case 'm':
			case 'M':
				strcpy(out_msg, "Printing Menu... \n");
				printMenu();
				break;
			case ' ':
				//timer_delete(timerid);
				strcpy(out_msg, "ESTOP pressed. \n");
				instr = ESTOP;
				break;
			case 'B':
				strcpy(out_msg,"BaseCW pressed. \n");
				instr = BASECW;
				break;
			case 'b':
				strcpy(out_msg,"BaseCCW pressed. \n");
				instr = BASECCW;	
				break;
			case 'S':
				strcpy(out_msg,"ShoulderUp pressed. \n");
				instr = SHOULDERUP;
				break;
			case 's':
				strcpy(out_msg,"ShoulderDown pressed. \n");
				instr = SHOULDERDW;
				break;
			case 'E':
				strcpy(out_msg,"ElbowUp pressed. \n");
				instr = ELBOWUP;
				break;
			case 'e':
				strcpy(out_msg,"ElbowDown pressed. \n");
				instr = ELBOWDW;
				break;
			case 'P':
				strcpy(out_msg,"PitchUp pressed. \n");
				instr = PITCHUP;
				break;
			case 'p':
				strcpy(out_msg,"PitchDown pressed. \n");
				instr = PITCHDW;
				break;
			case 'R':
				strcpy(out_msg,"RollCW pressed. \n");
				instr = ROLLCW;
				break;
			case 'r':
				strcpy(out_msg,"RollCCW pressed. \n");
				instr = ROLLCCW;
				break;
			case 'G':
				strcpy(out_msg,"GripperOpen pressed. \n");
				instr = GRIPPEROP;
				break;
			case 'g':
				strcpy(out_msg,"GripperClose pressed. \n");
				instr = GRIPPERCLOSE;
				break;
			case 'C':
				strcpy(out_msg,"ConveyorOn pressed. \n");
				instr = CONVEYORON;
				break;
			case 'c':
				strcpy(out_msg,"ConveyorOFF pressed. \n");
				instr = CONVEYOROFF;
				break;
			case 'L':
				strcpy(out_msg,"UnloadLeft pressed. \n");
				instr = UNLOADLEFT;
				break;
			case 'l':
				strcpy(out_msg,"UnloadRight pressed. \n");
				instr = UNLOADRIGHT;
				break;
			case 'H':
			case 'h':
				strcpy(out_msg,"Home pressed. \n");
				instr = HOMEROBOT;
				break;
			case 'A':
				strcpy(out_msg,"AutomodeOn pressed. \n");
				instr = MANUALOFF;
				break;
			case 'a':
				strcpy(out_msg,"AutomodeOff pressed. \n");
				instr = MANUALON;
				break;
			case 'D':
			case 'd':
				strcpy(out_msg,"Display pressed. \n");
				readMsg(0,1);
				break;
			default:
				break;
		}
		if (out_msg[0]  != 'o')
		{
			printf(out_msg);
			flushall();
		}
		if (instr != 100)
		{
			last_input = instr;
			writeMsg(instr);
		}
		//start the timer
		if ((kbInput == 'B')||(kbInput == 'b')||(kbInput == 'S')||(kbInput == 's')||
			(kbInput == 'E')||(kbInput == 'e')||(kbInput == 'P')||(kbInput == 'p')||
			(kbInput == 'R')||(kbInput == 'r')||(kbInput == 'G')||(kbInput == 'g')||
			(kbInput == 'L')||(kbInput == 'l'))
		{
			timer_settime(timerid, 0, &timer,NULL);
		}
		delay(100);
	}
	return EXIT_SUCCESS;
}


//----Block Monitoring Thread
void* blockID()
{	
	//---------------------------------------------
	//local variable		
	int dataCur[6] = {NULL};
	int	dataPre[6] = {1};
	int	edge[6] = {0};
	
	int pCountA,pCountB1,pCountB4,pCountC;
	int blockCountA,blockCountB1,blockCountC;

	int blockIndexB1, blockIndexC;	

	int c3fall, c1fall;
	int c3rise, c2rise, c1rise;
	
	struct timespec timeQNX;
	struct timespec t_offset;
	float now;
	float pass_now;
	float diff_time;
	float past_a_time;
	
	int time_check_B1_re = 0;
	int time_check_C_re = 0;
	
	
	
	//temporary variable
	int i, j, k;
	unsigned long long count;
	
	int block = 0;
	
	//special CONSTANT:
//	int conveyor = lowOutput.conveyor_ON_nOFF;
	int blockLength = 135;
	
	int endPortionA = blockLength;// - 15;
	int sectionInterval = blockLength/4;
	
	int sectionNumA = 0;
	int sectionNumB1 = 0;
	int sectionNumB4 = 0;
	int sectionNumC = 0;
	
	int exp_A2B = 5;
	int exp_A2C = 8;
	int exp_B2C = 3;//using B1 sensor
//	int exp_C2D = 5;
	
	float zeros[4][4] = {34.1};
	
	//store the accumulated data for different section
	float tempB = 0;
	float tempC3 = 0;
	float tempC2 = 0;
	float tempC1 = 0;
	
	int evenOdd = 0;
	
	block_t blockIDP[10];
	
	pattern();
	
//	int exp_A2B1=5;
//	int exp_B12B4 = 1;
//	int exp_B42C=2;
//	int exp_C2D=5;

	//----------------------------------------------
	//initialization:
	 pCountA =pCountB1 =pCountB4 =pCountC=0;
	 blockCountA =blockCountB1 =blockCountC=0;

	 blockIndexB1 = blockIndexC=0;

	 c3fall = c1fall=0;
	 c3rise = c2rise = c1rise=0;
	 
	 for(i = 0;i<10;i++)
	 {	 	
	 	blockIDP[i].expArr_B = 0;
	 	blockIDP[i].expArr_C = 0;
	 	blockIDP[i].status = EMPTY;
	 	blockIDP[i].progress = READY;;
	 }
	 
	 
	 
	 //get the offset time
	 
	 if( clock_gettime(CLOCK_REALTIME, &t_offset) == -1 )
	 {
			perror( "clock gettime" );
    		//return EXIT_FAILURE;
     }
     past_a_time = 0;
     sleep(1);
	 
	 while(killFlg == 0)
	 {
	 	if(lowOutput.conveyor_ON_nOFF == ON)// 1 = 0N
	 	{
	 		//------------------------------------------------------------
			// get the sensor data: a2, b1, b4, c3, c2, c1
			//------------------------------------------------------------
			readMsg(0,0);
			dataCur[0] = readLowerByte.sensor_nA2;
			dataCur[1] = readLowerByte.sensor_nB1;
			dataCur[2] = readLowerByte.sensor_nB4;
			dataCur[3] = readLowerByte.sensor_nC3;
			dataCur[4] = readLowerByte.sensor_nC2;
			dataCur[5] = readLowerByte.sensor_nC1;
			//dataCur = readSamples();
			// get the edge condition
			for(i = 0;i<6;i++)
			{
				edge[i] = dataCur[i] - dataPre[i];	
				dataPre[i] = dataCur[i];			
			}			
			//-------------------------------------------------------------
			if( clock_gettime( CLOCK_REALTIME, &timeQNX) == -1 )
			{
				perror( "clock gettime" );
      			//return EXIT_FAILURE;
    		}
    		now = timeQNX.tv_sec-t_offset.tv_sec+(double)timeQNX.tv_nsec/BILLION;
    		
			//--------------------------------------------------------------
			// analysis data at sensor A
			//--------------------------------------------------------------
			if(pCountA == 0)
			{
				//find the first falling edge
				if(edge[0] == -1)
				{
					

					blockIDP[blockCountA%10].status = NORMAL;					
					blockIDP[blockCountA%10].progress = HEADING_B;
					blockIDP[blockCountA%10].expArr_B = now + exp_A2B;
					blockIDP[blockCountA%10].expArr_C = now + exp_A2C;
					
//					printf("BM: Block A Index: %i.\n",blockCountA%10);
					printf("BM: Sensor A detects a block at %2.3f.\n",now);
//					printf("BM: The block expect arriving at Sensor B1 at %2.3f.\n\n",blockIDP[blockCountA%10].expArr_B);					
					flushall();
					
//					printf("BM: Time Difference: %f.\n",now-past_a_time);
					
					if((now-past_a_time)<2)
					{
						printf("=============================================\n");
						printf("BM:       Warning!                           \n");
						printf("  The distance between the consecutive blocks\n");
						printf("  that just passed sensor A is too short,    \n");
						printf("  The block may not be able to be properly   \n");
						printf("  identified.\n");
						printf("=============================================\n");
						flushall();
					}
					
					past_a_time = now;
					
					blockCountA++;					
					pCountA++;	
				}
			}
			else if(pCountA >= endPortionA)
			{
				if(dataCur[0] == 1)
				{
					pCountA = 0;//reset the count
				}
				else if((pCountA == endPortionA+3)&&(dataCur[0] == 0))
				{
					printf("=============================================\n");
					printf("BM:       Warning!                           \n");
					printf("  The distance between the consecutive blocks\n");
					printf("  that just passed sensor A is too short,    \n");
					printf("  The block may not be able to be properly   \n");
					printf("  identified.\n");
					printf("=============================================\n");
					pCountA = 0;//reset the count
				}
				else
				{
					pCountA++;
				}
			}
//			else if(pCountA > endPortionA-30)
//			{
//			//	pCountA = 0;//reset the count
//			//	find the last rising edge				
//				if(edge[0] == 1)
//				{				
//					printf("Block Monitor: The block passed SensorA. \n");
//					printf("BM: Total Num of Sample is %i. \n",pCountA);
//					flushall();				
//					pCountA = 0;//reset the count					
//				}
//				else
//				{pCountA++;}	
//			}
			else
			{
				pCountA++;
			}			
			//--------------------------------------------------------------
	 		//--------------------------------------------------------------
			// analysis data at sensor B1
			//--------------------------------------------------------------
			if(pCountB1 == 0)
			{
				//find the first falling edge
				if(edge[1] == FALLING)
				{				
					
					sectionNumB1 = 0;					
					blockIndexB1 = blockCountB1%10;	
					printf("BM: Sensor B dectect a block at %2.3f. \n",now);
					flushall();
					//check Block status detect block insertion
					if(!(blockIDP[blockIndexB1].status == NORMAL && blockIDP[blockIndexB1].progress == HEADING_B
					&& now >= blockIDP[blockIndexB1].expArr_B-1) )
					{
						printf("====================================================\n");
						printf("BM: A Block has been inserted between sensor A and B\n");
//						printf("    Block Index B1: %i\n",blockIndexB1);
						printf("====================================================\n");
						flushall();
						
					}
					else
					{					
					blockIDP[blockIndexB1].status = NORMAL;
					blockIDP[blockIndexB1].progress = HEADING_C;					
					blockIDP[blockIndexB1].expArr_C = now + exp_B2C;	
//					printf("BM: The block expect arriving at Sensor C at %2.3f. \n\n",blockIDP[blockIndexB1].expArr_C);
					flushall();				
					}
										
					tempB = dataCur[1];
					pCountB1++;	
				}
			}
			else
			{
				if(sectionNumB1<4)
				{
					if(pCountB1%sectionInterval == 0)
					{
						tempB = tempB/sectionInterval;
						blockIDP[blockIndexB1].senData[0][sectionNumB1] = tempB;
						tempB = 0;
						sectionNumB1++;
						pCountB1++;
					}
				tempB = tempB + dataCur[1];
				pCountB1++;
				}
				else
				{
//					printf("BM: A block pass B.Block data:\n");
//					flushall();
					pCountB1=0;		//reset the counter, and monitor next block			
				}					
			}
			//---------------------------------------------------------------
			//---------------------------------------------------------------
			// analysis data at sensor C
			//---------------------------------------------------------------
			if(pCountC == 0)
			{
				//find the first -1 edge
				if(edge[3] == -1)
				{
					c3fall = 1;
				}
				if(edge[5] == -1)
				{
					c1fall = 1;
				}

				if((c3fall + c1fall) == 2) // trigger by sensor C3 and C1
				{
					c3fall = c1fall = 0;// reset trigger condition
					
					blockIndexC = blockCountC%10;
						
					printf("BM: Sensor C dectect a block at %2.3f. \n",now);
					flushall();
					if(!(blockIDP[blockIndexC].status == NORMAL && blockIDP[blockIndexC].progress == HEADING_C
					&& now >= blockIDP[blockIndexC].expArr_C-0.5) )
					{
						blockIDP[blockIndexC].cstatus = BLOCK_ADDED;	
						printf("====================================================\n");
						printf("BM: An unexpected block is detected at sensor C\n.");
						printf("    It will be treated as an INVALID block!\n.This Block will be unloaded to the Left Bin\n");
//						printf("    Block Index C is %i\n",blockIndexC);
						printf("====================================================\n");
						flushall();
					}
					else
					{					
					 blockIDP[blockIndexC].status = NORMAL;
					 blockIDP[blockIndexC].progress = ARRIVE_C;
					}					
									
					sectionNumC=0;
					//increase the block count
//					blockCountC++;
					pCountC++;					
										
					
					
					tempC3 = dataCur[3];
					tempC2 = dataCur[4];
					tempC1 = dataCur[5];
				}

			}
			else
			{
				if(sectionNumC<4)
				{
					if(pCountC%sectionInterval == 0)
					{
						tempC3 = tempC3/sectionInterval;
						tempC2 = tempC2/sectionInterval;
						tempC1 = tempC1/sectionInterval;
						
						blockIDP[blockIndexC].senData[1][sectionNumC] = tempC3;
						blockIDP[blockIndexC].senData[2][sectionNumC] = tempC2;
						blockIDP[blockIndexC].senData[3][sectionNumC] = tempC1;
						
						tempC3 = 0;
						tempC2 = 0;
						tempC1 = 0;
						
						sectionNumC++;						
					}
					tempC3 = tempC3 + dataCur[3];
					tempC2 = tempC2 + dataCur[4];
					tempC1 = tempC1 + dataCur[5];
				
					pCountC++;
				}
				else
				{
					printf("\nBM: The block pass C.Block data:\n");					
					
					flushall();
					
					for(i = 0;i<4;i++)
					{
						for(j = 0;j<4;j++)
						{
							printf("%f, ",blockIDP[blockIndexC].senData[i][j]);
							//fprintf(pFile,"%f,",blockIDP[blockIndexC].senData[i][j]);
							flushall();
						}
						printf("\n");
						//fprintf(pFile,"\n");
						flushall();
					}
					printf("\n");
					//fprintf(pFile,"\n");
					flushall();
					block = findBlock(blockIDP[blockIndexC]);
					
					blockNumG = block;	
					
					evenOdd = block%2;		
					if(evenOdd == 1)//odd-->right
					{
						writeMsg(UNLOADRIGHT);
						printf("Block %i is Odd, unload to the right\n\n",block);
						flushall();
			 
					}
					else
					{
						writeMsg(UNLOADLEFT);
						printf("Block %i is Even, unload to the left\n\n",block);
						flushall();
					}		
		timer_settime(timerid,0, &timer,NULL);				
					
					if(block != BLOCK_ADDED && block != -1)
					{		
						printf("=============================\n");				
						printf("BM: Block %i is detected.\n",block);
						printf("=============================\n\n");
						flushall();
						blockIDP[blockIndexC].expArr_B = 0;
	 					blockIDP[blockIndexC].expArr_C = 0;
	 					blockIDP[blockIndexC].status = EMPTY;
	 					blockIDP[blockIndexC].progress = READY;	
					}
					else if (block == -1)
					{
						printf("BM: the block cannot be indentified.\n\n",block);
						flushall(); 
						blockIDP[blockIndexC].expArr_B = 0;
	 					blockIDP[blockIndexC].expArr_C = 0;
	 					blockIDP[blockIndexC].status = EMPTY;
	 					blockIDP[blockIndexC].progress = READY;	
					}
					else if (block == BLOCK_ADDED)
					{
						blockIDP[blockIndexC].cstatus = 0;
					}
					//Clear Buffer
														

					pCountC=0;	//reset the counter, and monitor next block			
				}
			}
			// Examine the time stamp for BLOCK removal
			//----------------------------------------------------------				
			//test for block removal between sensor A and B
			if (now > (blockIDP[blockCountB1%10].expArr_B+0.5-0.25)&&now <= (blockIDP[blockCountB1%10].expArr_B+0.5+0.25) &&(time_check_B1_re == 0))
			{
				time_check_B1_re = 1;
//				printf("BM: blockIDP B1 index  is %i\n",blockCountB1%10);
				flushall();
				if(blockIDP[blockCountB1%10].progress == HEADING_B)
				{
					//clear the block data buffer
					blockIDP[blockCountB1%10].status = BLOCK_REMOVED;
					blockIDP[blockCountB1%10].progress = EMPTY;
					
					printf("===================================================\n");
					printf("BM: A block that heads towards Sensor B is missing!\n");
//					printf("BM: The missing block index is %i.\n",blockCountB1%10);
					printf("===================================================\n");					
					flushall();								
				}				
			}
			else if (now > blockIDP[blockCountB1%10].expArr_B+0.5+0.25&&time_check_B1_re == 1)
			{

				time_check_B1_re = 0;
				//printf("BM: block B1 index %i, progress status%i\n",blockCountB1%10, blockIDP[blockCountB1%10].progress);
				//flushall();
				if(blockIDP[blockCountB1%10].status == BLOCK_REMOVED)
				{
					//NULL
														
				}
				else
				{
					//printf("BM: Block Count B: %i\n",blockCountB1);
					//flushall();
					
				}
				blockCountB1++;
			}
			
			
			//==================================================================
			//test for block removal between sensor B and C
			if (now > (blockIDP[blockCountC%10].expArr_C+0.5-0.25)&&now <= (blockIDP[blockCountC%10].expArr_C+0.5+0.25) &&(time_check_C_re == 0))
			{
				time_check_C_re = 1;
//				printf("BM: blockIDP C index  is %i\n",blockCountC%10);
				flushall();
				if(blockIDP[blockCountC%10].progress == HEADING_C ||blockIDP[blockCountC%10].status == BLOCK_REMOVED)
				{
					//clear the block data buffer					
					if(blockIDP[blockCountC%10].progress == HEADING_C)
					{
					printf("===================================================\n");
					printf("BM: A block that heads towards Sensor 'C' is missing!\n");
//					printf("BM: The missing block index is %i.\n",blockCountC%10);
					printf("===================================================\n");					
					flushall();
					}
					
					blockIDP[blockCountC%10].status = BLOCK_REMOVED;
					blockIDP[blockCountC%10].progress = EMPTY;								
				}				
			}
			else if (now > blockIDP[blockCountC%10].expArr_C+0.5+0.25&&time_check_C_re == 1)
			{
				time_check_C_re = 0;
				//printf("BM: block index at 'C' %i, progress status: %i\n",blockCountC%10, blockIDP[blockCountC%10].progress);
				//flushall();
				if(blockIDP[blockCountC%10].status == BLOCK_REMOVED)
				{
					blockIDP[blockCountC%10].expArr_C = blockIDP[blockCountC%10].expArr_C = 0;
					for(i = 0;i<4;i++)
					{
						for(j = 0;j<4;j++)
						{
							blockIDP[blockCountC%10].senData[i][j] = 1;							
						}
					}								
				}
				else
				{
					//printf("BM: Block Count C: %i\n",blockCountC);
					//flushall();
				}
				blockCountC++;
			}
			delay(10);		
	 		
	 	}
	 	
	 	else
	 	{
	 		if( clock_gettime( CLOCK_REALTIME, &timeQNX) == -1 )
			{
				perror( "clock gettime" );
      			//return EXIT_FAILURE;
    		}
    		pass_now = timeQNX.tv_sec-t_offset.tv_sec+(double)timeQNX.tv_nsec/BILLION;
    		diff_time = pass_now - now;
	 		delay(10);	 		
	 		for(i=0;i<10;i++)
	 		{
	 			if(blockIDP[i].status == NORMAL||blockIDP[i].status == BLOCK_REMOVED)
	 			{
	 				blockIDP[i].expArr_B = blockIDP[i].expArr_B+diff_time;
	 				blockIDP[i].expArr_C = blockIDP[i].expArr_C+diff_time;
	 			} 				
	 		}
	 		now = pass_now;	 		
	 	}	 	
	 }			
}

int findBlock(block_t block)
{
	int i,j,k,l;
	float min_MSE = 99999;
	float MSE = 99;
	int blockNum = 99;	
	float tempL = 0;
	//float dM[4][4] = {NULL};	
	
	//memcpy(dM,block.senData,sizeof(dM));	

	if (block.cstatus == BLOCK_ADDED)
	{
		return -2;//-2
	}
////-----Get Pattern
//	for(i=0;i<4;i++)
//	{
//		for(j = 0; j<4;j++)
//		{
////			printf("%f,\t",block.senData[i][j]);
//			fprintf(pFile,"%f,",block.senData[i][j]);			
//		}
////		printf("\n");
//		fprintf(pFile,"\n");
//		flushall();
//	}
////	printf("\n");
//	fprintf(pFile,"\n");
//	flushall();

////---------------------------------------------------
	if(programMode == '1')
	{
	
	
		for(k = 0;k<11;k++)
		{
			tempL = 0;
			for(i = 0;i<4;i++)
			{
				for(j = 0;j<4;j++)
				{
					tempL = tempL +(block.senData[i][j]-blockPa1[k][i][j])*(block.senData[i][j]-blockPa1[k][i][j]);
					//printf("temp: %f\n",tempL);
					//flushall();					
				}
			}
			tempL = (float) tempL/16;
			if(tempL<=min_MSE)
			{
				min_MSE = tempL;
				blockNum = k;
			}				
		}
	}
	else if (programMode == '2')
	{
		for(l = 0;l<4;l++)
		{
			for(k = 0;k<7;k++)
			{
				tempL = 0;
				for(i = 0;i<4;i++)
				{
					for(j = 0;j<4;j++)
					{
						tempL = tempL +(block.senData[i][j]-blockPa2[l][k][i][j])*(block.senData[i][j]-blockPa2[l][k][i][j]);
						//printf("temp: %f\n",tempL);
						//flushall();					
					}
				}
				tempL = (float) tempL/16;
				if(tempL<=min_MSE)
				{
					min_MSE = tempL;
					blockNum = k;
				}				
			}
		}
	}
	flushall();	
//	printf("MIN_MSE: %f\n",min_MSE);
	if(min_MSE>0.01)
	{
		blockNum = -1;
	}
	//printf("Best Matching Box: %i\n",blockNum);
	flushall();	
	return blockNum;

}

void pattern()
{
	
    //Read data from text file
	int i,j,k,l;
	FILE *f = fopen("blockPattern1.txt","rt");	
	for(k  = 0;k<11;k++)
	{
		//redata
		for(i=0;i<4;i++)
		{
			//J
			for(j = 0; j<4;j++)
			{
				fscanf(f,"%f,",&blockPa1[k][i][j]);
			}
		}
		//print data
		for(i=0;i<4;i++)
		{
			for(j = 0; j<4;j++)
			{
				printf("%f\t",blockPa1[k][i][j]);			
			}
			printf("\n");
		}
		printf("\n");
	}
	fclose(f);	
	f = fopen("blockPattern2.txt","rt");	
	
	//l is for block orientation
	for(l = 0;l<4;l++) 
	{
		//K is blocks number
		for(k  = 0;k<7;k++)
		{
			for(i=0;i<4;i++)
			{
				//J
				for(j = 0; j<4;j++)
				{
					fscanf(f,"%f,",&blockPa2[l][k][i][j]);
				}
			}
			for(i=0;i<4;i++)
			{
				for(j = 0; j<4;j++)
				{
					printf("%f\t",blockPa2[l][k][i][j]);			
				}
				printf("\n");
			}
			printf("\n");
		}
	}
	fclose(f);
	
}