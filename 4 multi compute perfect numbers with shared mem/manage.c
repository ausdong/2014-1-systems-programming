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
union semun {
	int val;			/* value for SETVAL */
	struct semid_ds *buf;	/* buffer for IPC_STAT & IPC_SET */
	unsigned short *array;	/* array for GETALL & SETALL */
};
#define KEY (key_t)92718   //key for shared memory
#define MKEY (key_t)99061 //key for message queue
#define SKEY (key_t)22568

int msgid; //msg queue id
int sid; // id of shared memory segment
int ssid;
int i; //for loops
//next three should be contained in the shared memory segment
unsigned char *bitmap; //the bitmap
int *perfectNum;	//this will be array to contain found perfect numbers
process *processList; //this will be array of "process" structures
computeMSG msg;
int totalComputes = 0;
int totalPerfectNums = 0;
pid_t activeComputes[20];
pid_t *managePID;

void terminate(void)
{
	//send interrupt to all compute
	for(i=0; i<20; i++)
	{
		if((*(processList+i)).pid == 0)
			break;
		kill(activeComputes[i], SIGINT);
	}

	//sleep 5s
	sleep(5);

	//detach shm
	shmdt((char *) bitmap);
	//deallocate shm and msgqueue
	shmctl(sid,IPC_RMID,0);
	msgctl(msgid,IPC_RMID,0);
	semctl(ssid,0,IPC_RMID,0);
	//terminate self
	printf("manage exiting\n");
	exit(0);
}


void handler(int signum)
{
	terminate();
}
 
int handle_signals(void)
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

//update array of perfect numbers, requested by compute
int updatePerfArray(int perfect)
{
		perfectNum[totalPerfectNums] = perfect;
		totalPerfectNums++;
}

//manager must initialize compute process entry
int intializeComputeEntry(int totalComputes, pid_t newPID)
{
	(*(processList+totalComputes-1)).pid = newPID;
	(*(processList+totalComputes-1)).numPerfect = 0;
	(*(processList+totalComputes-1)).numTested = 0;
	activeComputes[totalComputes-1] = newPID;
	(*(processList+totalComputes-1)).numSkip = 256000;
	/*for(i=0; i < totalComputes; i++)
	{	
		(*(processList+i)).numSkip = (int)(256000/totalComputes) - (*(processList+i)).numTested;
		if((*(processList+i)).numSkip < 0) (*(processList+i)).numSkip = 0;
	}
	//in case of uneven division
	(*(processList+totalComputes-1)).numSkip = (int)(256000/totalComputes) + 256000%totalComputes;*/
	
}

int main()
{
	handle_signals();
	ssid = semget(SKEY, 20, IPC_CREAT | 0666);
	union semun arg;
	arg.val = 1;
	for(i = 0; i < 20; i++)
	{
		semctl(ssid, i, SETVAL, arg);
	}
	//create/get and set up shared memory segment
	sid = shmget(KEY, (32001)*sizeof(unsigned char) + 20*sizeof(int) + 20*sizeof(process) + sizeof(pid_t), IPC_CREAT | 0666);
	bitmap = (unsigned char*)shmat(sid, NULL, 0);
	initializeBM(bitmap);
	perfectNum = (int*)(bitmap + 32001);
	processList = (process*) (perfectNum + 20);
	managePID = (pid_t*)(processList + 20);
	*managePID = getpid(); //so report can get manage's PID from shared memory to send signal

	//message queue
	msgid = msgget(MKEY, IPC_CREAT | 0666);

	int msize = sizeof(msg)-4;
	while(1)
	{
		msgrcv(msgid, &msg, msize, -4, 0);
		//register compute	
		if(msg.mtype == 1)
		{
			totalComputes++;
			msg.message = totalComputes;
			msg.mtype = 5;
			msgsnd(msgid, &msg, msize, 0);
			if(totalComputes>20)
			{
				printf("already have 20 computes running, last compute not registered\n");
				totalComputes--;
			}
			else
			{
				intializeComputeEntry(totalComputes, msg.computePID);
			}


		}
		//update perfect number array
		else if(msg.mtype == 2)
		{
			updatePerfArray(msg.message);
		}
		//compute ending, 'remove' from activeComputes and decrement totalComputes
		/*else if(msg.mtype == 3)
		{
			for(i=0; i<totalComputes; i++)
			{
				if(activeComputes[i]==msg.message)
				{
					while(i<totalComputes-1)
					{
						activeComputes[i] = activeComputes[i+1];
						i++;
					}
					activeComputes[i]==0;
					break;
				}
			}
			totalComputes--;
		}*/
	}

	exit(0);
}

