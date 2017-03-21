/*THIS CODE IS MY OWN WORK, IT WAS WRITTEN WITHOUT CONSULTING A TUTOR OR CODE WRITTEN BY OTHER STUDENTS - Austin Dong*/

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>

int main(int argc, char *argv[])
{
	int child;
	int parsed[2];
	int sorted[2];
	int i;
	int status;
	pipe(parsed); //(note to self)read from 0, write to 1 
	pipe(sorted);
	FILE *intoSort;
	FILE *afterSort;
	unsigned char aString[4096];
	unsigned char aWord[4096];
	unsigned char words[100][100];
	
	switch(fork())
	{
		case 0: //child - sort process
			dup2(parsed[0], STDIN_FILENO);
			close(parsed[0]);
			close(parsed[1]);
			dup2(sorted[1], STDOUT_FILENO);
			dup2(sorted[1], STDERR_FILENO);
			close(sorted[0]);
			close(sorted[1]);
			execl("/usr/bin/sort", "sort", (char *)NULL);		
		default:
			break;
	}

	
	switch(fork())
	{
		case 0: //child - remove duplicates and output to stdout
			close(parsed[0]);
			close(parsed[1]);
			close(sorted[1]);
			afterSort = fdopen(sorted[0], "r");
			for(i=0; i<100; i++)
			{
				fgets(words[i], sizeof(words[i]), afterSort);
			}
			int testString = 0;
			for(i=1; i<100; i++)
			{
				if(strcmp(words[i], words[testString]) == 0) words[i][0] = '\0';
				else if(strcmp(words[i], words[testString]) < 0)
				{
					if(words[i][0] != '\0') testString = i;
				}
				else
				{
					if(words[testString][0] == '\0') testString = i;
				}
			}
			fclose(afterSort);
			for(i=0; i<100; i++)
			{
				if(words[i][0] != '\0') 
				{
					
					fputs(words[i],stdout);
					fflush(stdout);
				}
			}
			_exit(0);
	
		default: //parent - read from stdin and parse
			close(parsed[0]);
			close(sorted[0]);
			close(sorted[1]);
			intoSort = fdopen(parsed[1], "w");
			int len;
			while(fgets(aString, sizeof(aString), stdin))
			{
	
				len = strlen(aString);			
				int pos = 0;
				for(i=0; i<=len; i++)
				{
					if(isalpha(aString[i])) aWord[pos++] = tolower(aString[i]);
					else if(pos > 0)
					{
						aWord[pos++] = '\n';
						aWord[pos++] = '\0';
						fputs(aWord, intoSort);
						pos = 0;
					}
				}
			}
			fclose(intoSort);
			close(parsed[1]);
			while ((child = wait(&status)) != -1)
				{
		
				}

	}

}
