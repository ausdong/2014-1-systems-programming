/*THIS CODE IS MY OWN WORK, IT WAS WRITTEN WITHOUT CONSULTING A TUTOR OR CODE WRITTEN BY OTHER STUDENTS - Austin Dong*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <utime.h>
#include <ar.h>
#include <string.h>
#include <fcntl.h>
#include <dirent.h>

//used to assist in transferring data
unsigned char ibuffer[8192];

//convert string to long long (used for ar_hdr elements)
long long int s2ll(char *str, int strLen)
{
	char temp[128];
	memcpy(temp, str, strLen);
	temp[strLen]='\0';
	return atoll(temp);
}

//convert string to int (octal mode)
long s28i(char *str, int strLen)
{
	char temp[128];
	memcpy(temp, str, strLen);
	temp[strLen]='\0';
	return strtol(temp, NULL, 8);
}

//append a file
void appendf(int arFD, char *file)
{
	int fileFD;
	int bytes=0;
	int nameLen=0;
	struct ar_hdr f_hdr;
	struct stat fileStat;
	stat(file, &fileStat);
	fileFD = open(file,2);
	nameLen = strlen(file);
	memcpy(f_hdr.ar_name, file, nameLen);
	f_hdr.ar_name[nameLen]='/';
	memset(&f_hdr.ar_name[nameLen+1], ' ', sizeof(f_hdr.ar_name)-nameLen-1);
	sprintf(f_hdr.ar_date, "%-12u", (unsigned int)fileStat.st_atime);
	sprintf(f_hdr.ar_uid, "%-6u", (unsigned int)fileStat.st_uid);
	sprintf(f_hdr.ar_gid, "%-6u", (unsigned int)fileStat.st_gid);
	sprintf(f_hdr.ar_mode, "%-8o", (unsigned int)fileStat.st_mode);
	sprintf(f_hdr.ar_size, "%-10u", (unsigned int)fileStat.st_size);
	strncpy(f_hdr.ar_fmag, ARFMAG, sizeof(f_hdr.ar_fmag));
	write(arFD, &f_hdr, sizeof(f_hdr));
	//while there are still bytes to be copied
	while((bytes=read(fileFD, ibuffer, 8192))>0)
	{
		write(arFD, ibuffer, bytes);
	}
	if(fileStat.st_size%2) write(arFD, "\n", 1);
	close(fileFD);
	return;
}

//q, quickly append named files to archive
void qappend(int argc, char *argv[])
{
	struct stat fileStat;
	int arFD;
	int i;
	//check if archive doesn't exist
	if(stat(argv[2], &fileStat)<0)
	{
		//archive doesn't exist, so make one
		arFD = creat(argv[2], 0666);
		lseek(arFD, 0, SEEK_SET);
		write(arFD, ARMAG, SARMAG);
	}
	else 
	{
		//open existing archive
		arFD = open(argv[2], 2);
		lseek(arFD, 0, SEEK_END);
	}
	//append the files
	for(i=3; i<argc; ++i)
	{
		appendf(arFD, argv[i]);
	}
	close(arFD);
	return;
}

//x, extract named files
void extract(int argc, char *argv[])
{
	int arFD = open(argv[2], 2);
	int fileFD;
	int pos;
	int i;
	int readSize;
	int mode;
	char fileSize[11];
	char *s;
	size_t toWrite;
	size_t size;
	struct stat arFileStat;
	struct ar_hdr f_hdr;
	read(arFD, ibuffer, SARMAG);
	if(stat(argv[2], &arFileStat)<0)
	{
		printf("archive does not exist\n");
		return;
	}
	else if(memcmp(ibuffer, ARMAG, SARMAG))
	{
		printf("archive is in the wrong format\n");
		return;
	}
	else //specified archive is valid
	{
		pos = lseek(arFD, 0, SEEK_CUR);
	
		while(pos<arFileStat.st_size)
		{
			//get file info from archive
			read(arFD, f_hdr.ar_name, sizeof(f_hdr.ar_name));
			read(arFD, f_hdr.ar_date, sizeof(f_hdr.ar_date));
			read(arFD, f_hdr.ar_uid, sizeof(f_hdr.ar_uid));
			read(arFD, f_hdr.ar_gid, sizeof(f_hdr.ar_gid));
			read(arFD, f_hdr.ar_mode, sizeof(f_hdr.ar_mode));
			read(arFD, f_hdr.ar_size, sizeof(f_hdr.ar_size));
			read(arFD, f_hdr.ar_fmag, sizeof(f_hdr.ar_fmag));
			memcpy(fileSize, f_hdr.ar_size, sizeof(f_hdr.ar_size));
			fileSize[sizeof(fileSize)]='\0';
			toWrite = atoll(fileSize);
			size = toWrite;
			s = index(f_hdr.ar_name, '/');
			if(s<f_hdr.ar_date) *s='\0';
			//compare file to all file arguments
			for(i=3; i<argc; i++)
			{
				//if match is found
				if(!strcmp(f_hdr.ar_name, argv[i]))
				{
					//make file
					fileFD = creat(argv[i], 0666);
					//copy file content
					while((toWrite>0)&&((readSize=read(arFD, ibuffer, toWrite>8192?8192:toWrite))>0))
					{
						write(fileFD, ibuffer, readSize);
						toWrite-=readSize;
					}
					close(fileFD);
					//restore timestamps
					struct utimbuf fileTime;
					fileTime.actime = s2ll(f_hdr.ar_date, sizeof(f_hdr.ar_date));
					fileTime.modtime = s2ll(f_hdr.ar_date, sizeof(f_hdr.ar_date));
					utime(argv[i], &fileTime);
					//restore permissions
					int mode = s28i(f_hdr.ar_mode, sizeof(f_hdr.ar_mode));
					chmod(argv[i], mode);
					//empty argv[i] since only want to operate on first match
					memset(argv[i], 0, strlen(argv[i]));
					pos = lseek(arFD, -size, SEEK_CUR);
					break;				
				}
			}
			pos = lseek(arFD,(size+1)&(~1), SEEK_CUR);
		}
	}
	close(arFD);
	return;	
}

//t, print concise table
void concise(int argc, char *argv[])
{
	int arFD = open(argv[2], 2);
	int pos;
	char *s;
	size_t toWrite;
	size_t size;
	struct stat arFileStat;
	struct ar_hdr f_hdr;
	read(arFD, ibuffer, SARMAG);
	if(stat(argv[2], &arFileStat)<0)
	{
		printf("archive does not exist\n");
		return;
	}
	else if(memcmp(ibuffer, ARMAG, SARMAG))
	{
		printf("archive is in the wrong format\n");
		return;
	}
	else //specified archive is valid
	{
		pos = lseek(arFD, 0, SEEK_CUR);
	
		while(pos<arFileStat.st_size)
		{
			//get file info from archive
			read(arFD, f_hdr.ar_name, sizeof(f_hdr.ar_name));
			read(arFD, f_hdr.ar_date, sizeof(f_hdr.ar_date));
			read(arFD, f_hdr.ar_uid, sizeof(f_hdr.ar_uid));
			read(arFD, f_hdr.ar_gid, sizeof(f_hdr.ar_gid));
			read(arFD, f_hdr.ar_mode, sizeof(f_hdr.ar_mode));
			read(arFD, f_hdr.ar_size, sizeof(f_hdr.ar_size));
			read(arFD, f_hdr.ar_fmag, sizeof(f_hdr.ar_fmag));
			toWrite = s2ll(f_hdr.ar_size, sizeof(f_hdr.ar_size));
			size = toWrite;
			s = index(f_hdr.ar_name, '/');
			if(s<f_hdr.ar_date) *s='\0';
			//print name and continue
			printf("%s\n", f_hdr.ar_name);
			pos = lseek(arFD,(size+1)&(~1), SEEK_CUR);
		}
	}
	close(arFD);
	return;	
}

//A, append fall "regular" files in current directory except archive itself
void apendAll(int argc, char *argv[])
{
	DIR *d;
	struct dirent *dir;
	struct stat fileStat;
	struct stat arFileStat;
	int arFD = open(argv[2], 2);
	read(arFD, ibuffer, SARMAG);
	if(stat(argv[2], &arFileStat)<0)
	{
		printf("archive does not exist\n");
		return;
	}
	else if(memcmp(ibuffer, ARMAG, SARMAG))
	{
		printf("archive is in the wrong format\n");
		return;
	}
	else 
	{
		//open valid archive
		arFD = open(argv[2], 2);
		lseek(arFD, 0, SEEK_END);
	}
	//open current directory
	d = opendir(".");
	//read through directory entries
	while((dir=readdir(d))!=NULL)
	{
		stat(dir->d_name, &fileStat);
		//if entry is regular file && entry name does not match archive name, then append it
		if((S_ISREG(fileStat.st_mode))&&(strcmp(dir->d_name, argv[2])))
		{
			appendf(arFD, dir->d_name);
		}
	}
	closedir(d);
	close(arFD);
	return;	
}

int main(int argc, char *argv[])
{
	if(argc<3) return -1;
	if(!strcmp(argv[1], "q")) qappend(argc, argv);
	else if(!strcmp(argv[1], "x")) extract(argc, argv);
	else if(!strcmp(argv[1], "t")) concise(argc, argv);
	else if(!strcmp(argv[1], "A")) apendAll(argc, argv);
	else return -1;
}
