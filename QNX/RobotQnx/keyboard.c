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

#include "SCORBOT.H"
#include "PPORT.H"
#include "CONST.H"
#include "GLOB.H"

void 	writeMsg (int);
void 	readMsg();
void*	timer_thread(void*notused);
void* 	GUI_thread(void*notused);
void 	mainLoop(int);
char 	kb_getc(void);
void* 	keyboard_input(void*notused);

struct _pulse guiPulse;	// message pulse structure
int		guiPulseVal;
int		coidsim;

main (void)
{
	pid_t	pid_gui;
	pid_t	pid_sim;
	int		simChid;
	int		kill_gui;
	int		kill_sim;
	int		s_return;
	
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
	simChid = 1;
	sleep(1);
	// Spawing a process for the GUI and Simulator
	pid_gui = spawnl(P_NOWAIT, "/usr/local/bin/gui_g", "gui_g", NULL);
	pid_sim = spawnl(P_NOWAIT, "/usr/local/bin/newGUIPport_g", "sim", NULL);
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
//   	// Print out timer channel ID and connection ID
//   	printf("Timer Channel ID is:%i\n",timerChid);
//   	flushall();
//   	printf("Timer Connection ID is: %i\n",timerCoid);
//	flushall();
	//	Set up pulse event for delivery when  the first timer expires; pulse code = 8, pulse value  = 0;
	SIGEV_PULSE_INIT (&timerEvent, timerCoid, SIGEV_PULSE_PRIO_INHERIT, CODE_TIMER, VALUE_TIMER);
	// Create the timer and bind it to the event
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
	// Initial pulse_count
	pulse_count = 0;
	//----------------timer monitor thread--------------------
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
	//----start the keyboard thread----------------------------
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
	
	
//prompts user interface
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
	printf("                Please enter selection:\n");	
	
// --------------------GUI Monitor Thread---------------------
	pthread_create(NULL,&attr,GUI_thread,NULL);
// --------------------others---------------------------------
	// call mainLoop()
	mainLoop(coidsim);
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
    ChannelDestroy(guiChid);
    ChannelDestroy(simChid);
}	

// --------------------------------------------------------------------------
void writeMsg(int guiPulseVal)
{
	// Reset hi output set byte
	highOutput.hiOutputSetByte = 0;
	// Reset low output set byte except conveyor_ON_nOFF
	if (lowOutput.conveyor_ON_nOFF == OFF)
		lowOutput.lowOutputSetByte = 0;
	else
		lowOutput.lowOutputSetByte = 8;
	// msgType => write
	smsg.msgType = WRITE;
	switch (guiPulseVal)
	{
		// elbow joint control
		case (ELBOWUP):
		case (ELBOWDW):
			highOutput.elbow_ON_nOFF = ON;
			if (guiPulseVal == ELBOWUP)
				highOutput.elbowDir_UP_nDOWN = elbow_UP;
			else 
				highOutput.elbowDir_UP_nDOWN = elbow_DOWN;
			break;
		// gripper movement control
		case (GRIPPEROP):
		case (GRIPPERCLOSE):
			highOutput.gripper_ON_nOFF = ON;
			if (guiPulseVal == GRIPPEROP)
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
			if (guiPulseVal == BASECW)
				lowOutput.baseDir_RIGHT_nLEFT = baseDir_RIGHT;
			else
				lowOutput.baseDir_RIGHT_nLEFT = baseDir_LEFT;
			break;
		// shoulder joint control
		case (SHOULDERUP):
		case (SHOULDERDW):
			lowOutput.shoulder_ON_nOFF = ON;
			if (guiPulseVal == SHOULDERUP)
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
		case 16 :
			highOutput.elbow_ON_nOFF = OFF;
			highOutput.gripper_ON_nOFF = OFF;
			lowOutput.base_ON_nOFF = OFF;
			lowOutput.shoulder_ON_nOFF = OFF;
			lowOutput.unloaderRight_ON_nOFF = OFF;
			lowOutput.unloaderLeft_ON_nOFF = OFF;
			break;
	}
	
	smsg.rwdataMsg.upper_data = highOutput.hiOutputSetByte;
	smsg.rwdataMsg.lower_data = lowOutput.lowOutputSetByte;
	if(MsgSend(coidsim,&smsg,sizeof(smsg),NULL,NULL) == -1)
	{
		printf("Sim message send failed\n");
		flushall();
	}
}

// ------------------------------------------------------------------------
void readMsg()
{
	// msgType => write
	smsg.msgType = READ;
	// command the simulator to return system status
	MsgSend(coidsim,&smsg,sizeof(smsg),&rmsg,sizeof(rmsg));
	// extract system status data
	readUpperByte.hiInputSetByte = rmsg.rwdataMsg.upper_data;
	readLowerByte.lowInputSetByte = rmsg.rwdataMsg.lower_data;
	// Check whether system status has changed
	if ((memData.upperByte != readUpperByte.hiInputSetByte) || (memData.lowerByte != readLowerByte.lowInputSetByte))
	{
		// display interpretation
		if (readUpperByte.baseLimit_nBL == 0)
			printf("base working within range!\n");
		if (readUpperByte.shoulderLimit_nBL == 0)
			printf("shoudler outside limit!\n");
		if (readUpperByte.elbowLimit_nBL == 0)
			printf("elbow outside its limit!\n");
		if (readUpperByte.pitchLimit_nBL == 0)
			printf("pitch outside its limit!\n");
		if (readUpperByte.rollLimit_nBL == 0)
			printf(" roll outside its limit!\n");
		if (readUpperByte.unloader_nEMPTY == 0)
			printf(" unloader outside its limit!\n");
		if (readUpperByte.loader_nEMPTY == 0)
			printf("loader outside its limit!\n");
		if (readLowerByte.sensor_nA2 == 0)
			printf("A2 \"sees\" a silver surface\n");
		if (readLowerByte.sensor_nB4 == 0)
			printf("B4 \"sees\" a silver surface\n");
		if (readLowerByte.sensor_nB1 == 0)
			printf("B1 \"sees\" a silver surface\n");
		if (readLowerByte.sensor_nC3 == 0)
			printf("C3 \"sees\" a silver surface\n");
		if (readLowerByte.sensor_nC2 == 0)
			printf("C2 \"sees\" a silver surface\n");
		if (readLowerByte.sensor_nC1 == 0)
			printf("C1 \"sees\" a silver surface\n");
		printf("\n");
		flushall();
	}
}

// --------------------------------------------------------------------------
void* timer_thread(void*notused)
{
	int time_count;
	int rcvid_timer;
//	printf("Start to monitor the pluse!\n\n");
//	flushall();
	while(1)
	{	
		rcvid_timer = MsgReceivePulse(timerChid, &timerPulse, sizeof(timerPulse), NULL);
		if (rcvid_timer==-1)
		{
			printf("Error in timer pulse!\n");
			flushall();
		}
		else
		{
			pulse_count++;
			printf("Timer:2 second passed, stop current operation\n");
			flushall();
//		
//			printf("Timer:The pulse code is %i\npulse value is %i\n\n",timerPulse.code,timerPulse.value);
//			flushall();
		}
	}
}
// ---------------------------------------------------------------------------
void* GUI_thread(void*notused)
{
	highOutput.hiOutputSetByte = 0;
	lowOutput.lowOutputSetByte = 0;
	while(1)
	{
		// Receive message from GUI
		MsgReceivePulse (2,&guiPulse,sizeof(guiPulse),NULL);
		// initialize guiPulseVal
		guiPulseVal = 0;
		// extract the pulse value
		guiPulseVal = guiPulse.value.sival_int;
		// print GUI pulse
		printf("GUI: Pulse: %i \n", guiPulseVal);
		// Quit loop if guiPulseVal == GUIQUIT
		if (guiPulseVal == GUIQUIT) {killFlg = 1; break;}
		// Command simulator
		writeMsg(guiPulseVal);
	}
}
// --------------------------------------------------------------------------
void mainLoop(int coidsim)
{
	while(killFlg == 0)
	{	
/*
		// timer starter
		timer_settime(timerid,0, &timer,NULL);
		while(pulse_count<1)
		{
			printf("Timer expire after 2 sec!\n");
			break;				
		}
*/
		readMsg();
		// shared memory access
			// update system states in shared memory
			if (guiPulseVal == CONVEYORON)
				memData.conveyorOn = ON;
			else if (guiPulseVal == CONVEYOROFF)
				memData.conveyorOn = OFF;
			if (guiPulseVal == MANUALON)
				memData.autoModeOn = OFF;
			else if (guiPulseVal == MANUALOFF)
				memData.autoModeOn = ON;
			memData.upperByte = rmsg.rwdataMsg.upper_data;
			memData.lowerByte = rmsg.rwdataMsg.lower_data;
			// blkType will be renewed in task3
			// by now, just use it for testing
			memData.blkType = 2;
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
	if(size == -1){
		return 0;
	}else{
		return ch;
	}
}

void* keyboard_input(void* notuse)
{
	struct _pulse intrPulse;
	char kbInput = 0; //Stores keyboard input
//	printf(" keyboard_input thread created.\n");
//	flushall();	
	while(killFlg == 0){
		// Receive message from GUI
		//MsgReceivePulse (2,&guiPulse,sizeof(guiPulse),NULL);
		//guiPulseVal = guiPulse.value.sival_int;	// extract the pulse value
		//printf("Keyboard: guipulse is : %i\n",guiPulse.value.sival_int);
		kbInput = kb_getc();
			
		// quit program and reset all robot motors
		if( kbInput == 'q' || kbInput == 'Q')
		{
         	writeMsg(GUIQUIT); // send message to GUIport
			readMsg();
// perform all the necessary clean-up here! including killing the other 2 GUI processes
			printf("Program Exiting... \n");
			flushall();
			killFlg = 1;
			delay(30);
			exit(0);
		}
		else if( kbInput == ' ')
		{	
			printf("ESTOP pressed. \n");
// Set up message body as discussed in class and send it to GUIport	
			writeMsg(ESTOP);
			//readMsg(kb_simCoid);
	  	    delay(30);
			flushall();
		}
		else if ( kbInput == 'B'){
			printf("BaseCW pressed. \n");
			flushall();
			writeMsg(BASECW);
			readMsg();
			timer_settime(timerid,0, &timer,NULL);
			while(pulse_count<1)
			{
				delay(10);					
			}
			pulse_count = 0;
			writeMsg(ESTOP);
			delay(10);
		// Set up message body and send it to GUIport
		}
		else if( kbInput == 'b'){
			printf("BaseCCW pressed. \n");
			flushall();
			writeMsg(BASECCW);
			readMsg();
			timer_settime(timerid,0, &timer,NULL);
			while(pulse_count<1)
			{
				delay(10);					
			}
			pulse_count = 0;
			writeMsg(ESTOP);
			delay(30);
		}
		else if ( kbInput == 'S'){
			printf("ShoulderUp pressed. \n");
			flushall();
			writeMsg(SHOULDERUP);
			readMsg();
			timer_settime(timerid,0, &timer,NULL);
			while(pulse_count<1)
			{
				delay(10);					
			}
			pulse_count = 0;
			writeMsg(ESTOP);
			delay(10);		
		}
		else if( kbInput == 's'){
			printf("ShoulderDown pressed. \n");
			flushall();
			writeMsg(SHOULDERDW);
			readMsg();
			timer_settime(timerid,0, &timer,NULL);
			while(pulse_count<1)
			{
				delay(10);					
			}
			pulse_count = 0;
			writeMsg(ESTOP);
			delay(10);
		}
		else if ( kbInput == 'E'){
			printf("ElbowUp pressed. \n");
			flushall();
			writeMsg(ELBOWUP);
			readMsg();
			timer_settime(timerid,0, &timer,NULL);
			while(pulse_count<1)
			{
				delay(10);					
			}
			pulse_count = 0;
			writeMsg(ESTOP);
			delay(10);		
		}
		else if( kbInput == 'e'){
			printf("ElbowDown pressed. \n");
			flushall();
			writeMsg(ELBOWDW);
			readMsg();
			timer_settime(timerid,0, &timer,NULL);
			while(pulse_count<1)
			{
				delay(10);					
			}
			pulse_count = 0;
			writeMsg(ESTOP);
			delay(10);
		}
		else if ( kbInput == 'P'){
			printf("PitchUp pressed. \n");
			flushall();
			writeMsg(PITCHUP);
			readMsg();
			timer_settime(timerid,0, &timer,NULL);
			while(pulse_count<1)
			{
				delay(10);					
			}
			pulse_count = 0;
			writeMsg(ESTOP);
			delay(10);		
		}
		else if( kbInput == 'p'){
			printf("PitchDown pressed. \n");
			flushall();
			writeMsg(PITCHDW);
			readMsg();
			timer_settime(timerid,0, &timer,NULL);
			while(pulse_count<1)
			{
				delay(10);					
			}
			pulse_count = 0;
			writeMsg(ESTOP);
			delay(10);
		}
		else if ( kbInput == 'R'){
			printf("RollCW pressed. \n");
			flushall();
			writeMsg(ROLLCW);
			readMsg();
			timer_settime(timerid,0, &timer,NULL);
			while(pulse_count<1)
			{
				delay(10);					
			}
			pulse_count = 0;
			writeMsg(ESTOP);
			delay(10);		
		}
		else if( kbInput == 'r'){
			printf("RollCCW pressed. \n");
			flushall();
			writeMsg(ROLLCCW);
			readMsg();
			timer_settime(timerid,0, &timer,NULL);
			while(pulse_count<1)
			{
				delay(10);					
			}
			pulse_count = 0;
			writeMsg(ESTOP);
			delay(10);	
		}
		else if ( kbInput == 'G'){
			printf("GripperOpen pressed. \n");
			flushall();
			writeMsg(GRIPPEROP);
			readMsg();
			timer_settime(timerid,0, &timer,NULL);
			while(pulse_count<1)
			{
				delay(10);					
			}
			pulse_count = 0;
			writeMsg(ESTOP);
			delay(10);	
		}
		else if( kbInput == 'g'){
			printf("GripperClose pressed. \n");
			flushall();
			writeMsg(GRIPPERCLOSE);
			readMsg();
			timer_settime(timerid,0, &timer,NULL);
			while(pulse_count<1)
			{
				delay(10);					
			}
			pulse_count = 0;
			writeMsg(ESTOP);
			delay(10);	
		}
		else if ( kbInput == 'C'){
			printf("ConveyorOn pressed. \n");
			flushall();
			writeMsg(CONVEYORON);
			readMsg();
			guiPulseVal = CONVEYORON;
		}
		else if( kbInput == 'c'){
			printf("ConveyorOFF pressed. \n");
			flushall();
			writeMsg(CONVEYOROFF);
			readMsg();
			guiPulseVal = CONVEYOROFF;
		}
		else if ( kbInput == 'L'){
			printf("UnloadLeft pressed. \n");
			flushall();
			writeMsg(UNLOADLEFT);
			readMsg();
			delay(10);
			writeMsg(ESTOP);
		}
		else if ( kbInput == 'l'){
			printf("UnloadRight pressed. \n");
			flushall();
			writeMsg(UNLOADRIGHT);
			readMsg();
			delay(10);
			writeMsg(ESTOP);
		}
		else if ( kbInput == 'H' || kbInput == 'h'){
			printf("Home pressed. \n");
			flushall();
			writeMsg(HOMEROBOT);
			readMsg();
			delay(10);
			writeMsg(ESTOP);
		}
		else if ( kbInput == 'A'){
			printf("AutomodeOn pressed. \n");
			flushall();
			writeMsg(MANUALOFF);
			readMsg();
			guiPulseVal = MANUALON;
		}
		else if( kbInput == 'a'){
			printf("AutomodeOff pressed. \n");
			flushall();
			writeMsg(MANUALON);
			readMsg();
			guiPulseVal = MANUALOFF;
		}
		else if ( kbInput == 'D' || kbInput == 'd'){
			printf("Display pressed. \n");
			flushall();
			readMsg();
		}		
		// Receive message from GUI
        //	MsgReceivePulse (2,&guiPulse,sizeof(guiPulse),NULL);
//		guiPulseVal = guiPulse.value.sival_int;	// extract the pulse value
//		// print GUI pulse
//		printf("GUI: Pulse: %i \n", guiPulseVal);		
	}
	return EXIT_SUCCESS;
}