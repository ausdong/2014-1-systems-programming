/*THIS CODE IS MY OWN WORK, IT WAS WRITTEN WITHOUT CONSULTING A TUTOR OR CODE WRITTEN BY OTHER STUDENTS - Austin Dong*/

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include "header.h"

#define KEY (key_t)92718   //shm key

int sid; // id of shared memory segment
int i; //for loops
//next three should be contained in the shared memory segment
unsigned char *bitmap; //the bitmap
int *perfectNum;	//this will be array to contain found perfect numbers
process *processList; //this will be array of "process" structures
pid_t *managePID;	

int main(int argc, char *argv[])
{
	//create/get and set up shared memory segment
	sid = shmget(KEY, (256000/8+1)*sizeof(unsigned char) + 20*sizeof(int) + 20*sizeof(process) + sizeof(pid_t), IPC_CREAT | 0666);
	bitmap = (unsigned char*) shmat(sid,NULL,0);
	perfectNum = (int*)(bitmap + 32001);
	processList = (process*)(perfectNum + 20);
	managePID = (pid_t*)(processList + 20);
	
	printf("\n----------Beginning of a report----------\n");
	printf("||Computed perfect numbers: ");
	for(i=0; i<20; i++)
	{
		if(perfectNum[i] != 0)
			printf("%d\t", perfectNum[i]);
	}
	printf("\n");
	int totalTested = 0;
	for(i=0; i<20; i++)
	{
		if((*(processList+i)).numTested != 0)
			totalTested += processList[i].numTested;
	}
	printf("||Total numbers tested: %d.\n", totalTested);
	printf("||Info of computes:\n||(index), (pid), (# perfects found), (# tested), (# not tested)");
	for(i=0; i<20; i++)
	{
		if((*(processList+i)).pid == 0)
			break;
		printf("\n||%d,\t%d,\t%d,\t%d,\t%d", i, processList[i].pid, processList[i].numPerfect, processList[i].numTested, processList[i].numSkip);
	}
	printf("\n----------End of a report----------\n\n");
	if(argc == 2 && !strcmp(argv[1], "-k"))
	{
		kill(*managePID, SIGINT);
	}
}

