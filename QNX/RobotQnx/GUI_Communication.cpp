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

void writeMsg (int, int);         // invoked in GUI_thread
void readMsg(int);            // invoked in mainLoop
void* timer_thread(void*notused);
void* GUI_thread(void*notused); //write message is invoked inside this function
void mainLoop(int);

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
	shMem=shm_open("shared_memory", OFLAGS, 0777); //Argument1:The name of the shared memory object that you want to open,
	                                               // it returns: A nonnegative integer, which is the lowest numbered 
	                                               // unused file descriptor
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
   	// Print out timer channel ID and connection ID
   	printf("Timer Channel ID is:%i\n",timerChid);
   	flushall();
   	printf("Timer Connection ID is: %i\n",timerCoid);
	flushall();
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
// --------------------GUI Monitor Thread---------------------
	pthread_create(NULL,&attr,GUI_thread,NULL); // GUI monitor thread 





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
void writeMsg(int coidsim,int guiPulseVal)
{
	// Reset hioutput setbyte
	highOutput.hiOutputSetByte = 0;
	// Reset low output set byte except conveyor_ON_nOFF
	if (lowOutput.conveyor_ON_nOFF == OFF)
		lowOutput.lowOutputSetByte = 0;   // conveyor is OFF  00000000
	else
		lowOutput.lowOutputSetByte = 8;   // conveyor is ON   10000000

	// msgType => write
	smsg.msgType = WRITE;
	switch (guiPulseVal)
	{
		// elbow joint control
		case (ELBOWUP):          // either elbow up or down means elbow is ON
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
		case 16 :   // pulse 16 means mouse is released
			highOutput.elbow_ON_nOFF = OFF;
			highOutput.gripper_ON_nOFF = OFF;
			lowOutput.base_ON_nOFF = OFF;
			lowOutput.shoulder_ON_nOFF = OFF;
			lowOutput.unloaderRight_ON_nOFF = OFF;
			lowOutput.unloaderLeft_ON_nOFF = OFF;
			break;
	}

	//retrieving reply messages to be sent to simulator using Msgsend
	smsg.rwdataMsg.upper_data = highOutput.hiOutputSetByte;
	smsg.rwdataMsg.lower_data = lowOutput.lowOutputSetByte;

	if(MsgSend(coidsim,&smsg,sizeof(smsg),NULL,NULL) == -1)// command roboat arm( simulator) to do something
	{
		printf("Sim message send failed\n");
		flushall();
	}
}

// ------------------------------------------------------------------------
void readMsg(int coidsim)
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
	printf("Start to monitor the pluse!\n\n");
	flushall();
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
			printf("Received a Pulse!\nThe pulse count is %i\n",pulse_count);
			flushall();
		
			printf("The pulse code is %i\npulse value is %i\n\n",timerPulse.code,timerPulse.value);
			flushall();
		}
	}
}
// ---------------------------------------------------------------------------
void* GUI_thread(void*notused)
{
	highOutput.hiOutputSetByte = 0;//reset all actuators
	lowOutput.lowOutputSetByte = 0;// reset all actuators
	while(1)
	{
		// Receive message from GUI
		MsgReceivePulse (2,&guiPulse,sizeof(guiPulse),NULL); // returns 0 upon success
		// initialize guiPulseVal
		guiPulseVal = 0;
		// extract the pulse value
		guiPulseVal = guiPulse.value.sival_int;
		// print GUI pulse
		printf("GUI: Pulse: %i \n", guiPulseVal);
		// Quit loop if guiPulseVal == GUIQUIT
		if (guiPulseVal == GUIQUIT) 
		{killFlg = 1;
		break;
		}
		// Command simulator
		writeMsg(coidsim,guiPulseVal);
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
		readMsg(coidsim);
		// shared memory access
		sem_wait(sem);
			// update system states in shared memory
			if (guiPulseVal == CONVEYORON)
				memData.conveyorOn = ON;
			else if (guiPulseVal == CONVEYOROFF)
				memData.conveyorOn = OFF;
			if (guiPulseVal == MANUALON)
				memData.autoModeOn = OFF;
			else if (guiPulseVal == MANUALOFF)
				memData.autoModeOn = ON;

			// retrieving the reply messages.
			memData.upperByte = rmsg.rwdataMsg.upper_data;
			memData.lowerByte = rmsg.rwdataMsg.lower_data;

			// blkType will be renewed in task3
			// by now, just use it for testing
			memData.blkType = 2;
			*memLocation = memData;                  // using shared memory created previously, Mow is memData
		sem_post(sem);
		//  request status from GUIport and write to shared memory for every 50 ms
		delay(50);
	}
}