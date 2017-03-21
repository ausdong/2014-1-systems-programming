/*THIS CODE IS MY OWN WORK, IT WAS WRITTEN WITHOUT CONSULTING A TUTOR OR CODE WRITTEN BY OTHER STUDENTS - Austin Dong*/

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <fcntl.h>
#include "header.h"

#define KEY (key_t)92718   //key for shared memory
#define MKEY (key_t)99061 //key for message queue
#define SKEY (key_t) 22568 //key for semaphore

int msgid; //message queue id
int sid; // id of shared memory segment
int ssid; //id of semaphore
int i; //for loops
//next three are shared memory segment
unsigned char *bitmap; //the bitmap
int *perfectNum;	//array to contain found perfect numbers
process *processList; //array of "process" structures
computeMSG msgToManage;
int msize = sizeof(msgToManage)-4;
pid_t *managePID;

int startNum=0;
process *currProcessInfo;
int processIndex = -1 ;

struct sembuf op;

int semIndex = 0;

static void terminate(void)
{	
	if(processIndex == -1 || 20)
		exit(0);
	
	//'delete' process entry from shm
	for(i=processIndex; i<19; i++)
	{
		(*(processList+i)).pid = (*(processList+i+1)).pid;
		(*(processList+i)).numPerfect = (*(processList+i+1)).numPerfect;
		(*(processList+i)).numTested = (*(processList+i+1)).numTested;
		(*(processList+i)).numSkip = (*(processList+i+1)).numSkip;
	}
	(*(processList+19)).pid = 0;
	(*(processList+19)).numPerfect = 0;
	(*(processList+19)).numTested = 0;
	(*(processList+19)).numSkip = 0;

	exit(0);
}

static void handler(int signum)
{
	terminate();
}
 
static int handle_signals(void)
{
	sigset_t set;
	struct sigaction act;
	
	sigfillset(&set);
	sigprocmask(SIG_SETMASK, &set, NULL);
	memset(&act, 0, sizeof(act));
	sigfillset(&act.sa_mask);
	act.sa_handler = handler;
	sigaction(SIGINT, &act, NULL);
	sigaction(SIGQUIT, &act, NULL);
	sigaction(SIGHUP, &act, NULL);
	sigemptyset(&set);
	sigprocmask(SIG_SETMASK, &set, NULL);
	return 1;
}


int testPerfect(int num)
{
	int sum = 0;
	int currFactor;
	int result = 0;
	
	for(currFactor = 1; currFactor <= (num-1); currFactor++)
	{
		if((num%currFactor) == 0) sum += currFactor;
	}
	if(sum == num) return sum;
	else return 0;
}

int computePerfects()
{
	int bmIndex = 12800*processIndex;
	int number=0;
	while((*currProcessInfo).numSkip != 0)
	{
		op.sem_op = -1;
		semop(ssid, &op, 1);
		if(setBM(bmIndex, bitmap) == 0)
		{	
			op.sem_op = 1;
			semop(ssid, &op, 1);
			//setBM(bmIndex, bitmap);
			(*currProcessInfo).numSkip--;
			(*currProcessInfo).numTested++;
			if((number=testPerfect(startNum + bmIndex))>0)
			{
				(*currProcessInfo).numPerfect++;
				msgToManage.computePID = getpid();
				msgToManage.message = number;
				msgToManage.mtype = 2;
				msgsnd(msgid, &msgToManage, msize, 0);
			}
		}
		else {
			op.sem_op = 1;
			semop(ssid, &op, 1);
		}
		bmIndex++;
		if(bmIndex%12800 == 0) {
			semIndex++;
			op.sem_num = semIndex;
		}
		if(bmIndex >= 256000)
			return;
	}
}

int main(int argc, char *argv[])
{
	handle_signals();
	if(argc == 2) startNum = atoi(argv[1]);

	sid = shmget(KEY, (256000/8+1)*sizeof(unsigned char) + 20*sizeof(int) + 20*sizeof(process) + sizeof(pid_t), IPC_CREAT | 0666);
	ssid = semget(SKEY, 1, IPC_CREAT | 0666);
	op.sem_op = -1;
	op.sem_flg = 0;
	bitmap = (unsigned char*)shmat(sid, NULL, 0);
//	initializeBM(bitmap);
	perfectNum = (int*)(bitmap + 32001);
	processList = (process*) (perfectNum + 20);
	managePID = (pid_t*)(processList + 20);
	//get msg queue and register with manage
	msgid = msgget(MKEY, IPC_CREAT | 0666);
	msgToManage.computePID = getpid();
	msgToManage.mtype = 1;
	msgToManage.message = 0;
	msgsnd(msgid, &msgToManage, msize, 0);
	msgrcv(msgid, &msgToManage, msize, 5, 0);
	processIndex = msgToManage.message-1;
	if(processIndex == 20) exit(0);
	op.sem_num = 12800*processIndex;
	currProcessInfo = processList+processIndex;
	
	//begin computing perfect numbers
	computePerfects();
	
	/*msgToManage.mtype = 3;
	msgToManage.message = 0;
	msgToManage.computePID = getpid();
	msgsnd(msgid, &msgToManage, msize, 0);*/
	exit(0);
}

