#ifndef _GLOB_H_
#define _GLOB_H_

#include <semaphore.h>

struct shareData{
	uint8_t lowerByte;
	uint8_t upperByte;
	int blkType;
	int conveyorOn;
	int autoModeOn;
};

struct msgContents smsg;	
struct msgContents rmsg;	

pthread_t inputTrackerThrdID;

hiOutputSet highOutput;		//upper bit output data for the scorbot
lowOutputSet lowOutput;	


int killFlg;
int coidPport;
int coidGui;
int pportChid;
int guiChid;
int intrChid;
int ArmMoveTime [2];

hiInputSet readUpperByte;
lowInputSet readLowerByte;

FILE *timing_data;
//---------Semaphore------------------//
sem_t *sem;
int shMem;
int *shmLocation;
struct shareData *memLocation;
struct shareData memData;

//---------User Define Variable------//
//---2 second Timer------------------//
int timerCoid;
int timerChid;


struct sigevent timerEvent;
struct _pulse timerPulse;
struct itimerspec timer;

timer_t timerid;

sig_atomic_t signal_count;
sig_atomic_t pulse_count;



//---thread paramters--------------//
pthread_attr_t attr;
pthread_t timerThreadID;
pthread_t keyboardThreadID;

#endif
