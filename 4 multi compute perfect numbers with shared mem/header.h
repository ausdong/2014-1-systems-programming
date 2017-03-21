/*THIS CODE IS MY OWN WORK, IT WAS WRITTEN WITHOUT CONSULTING A TUTOR OR CODE WRITTEN BY OTHER STUDENTS - Austin Dong*/

#ifndef HEADER_H_GUARD
#define HEADER_H_GUARD

#include <stdio.h>   
#include <stdlib.h>   
#include <string.h>   
   
int initializeBM(unsigned char *BM )
{
	BM = (unsigned char*)malloc(32001* sizeof(unsigned char));
	memset(BM, 0, 32001);
	return 1;
}

int setBM(int index, unsigned char *BM)
{
	int quo = index / 8;
	int remainder = (8-(index+1)%8)%8;
	unsigned char x = (0x1<<remainder);
	int aset = BM[quo]&x;
	BM[quo]|= x;
	return aset>0 ? 1:0;
}

int getBM(int index, unsigned char *BM)
{
	int quo = index / 8;
	int remainder =(8- (index+1) % 8)%8;
	unsigned char x = (0x1<<remainder);
	unsigned char res;
	res = BM[quo] & x;
	return res>0 ? 1:0;
}

typedef struct{
	pid_t pid;
	int numPerfect;
	int numTested;
	int numSkip;
}process;


typedef struct{
	long mtype;
	pid_t computePID;
	int message;
}computeMSG;


#endif
