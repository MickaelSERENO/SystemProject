#define _GNU_SOURCE
#include "signal.h"
#include "unistd.h"
#include "stdio.h"
#include "stdlib.h"
#include "sys/wait.h"
#include "stdint.h"
#include "string.h"
#include "unistd.h"
#include "errno.h"
#include "fcntl.h"
#include "utime.h"
#include "sys/types.h"
#include "sys/stat.h"

#define BUFFER_SIZE 1023

pid_t childID=0;
uint32_t interrupt=0;
void childThread(char** argv);
void getArgv(char* buffer, char** argv);
void getSigInt(int sig);
void handle_int(int num);

int main()
{
	char currentDir[BUFFER_SIZE+1];
	char** history=NULL; 
	uint32_t historyLen=0;
	getcwd(currentDir, BUFFER_SIZE);
	uint8_t exit=0;

	struct sigaction int_handler = {.sa_handler=handle_int};
	int_handler.sa_flags = SA_NOCLDSTOP | SA_NOCLDWAIT;
	sigaction(SIGINT,&int_handler,0);

	while(!exit)
	{
		printf("%s >", currentDir);
		fflush(stdout);
		char* buffer=NULL;
		uint32_t nbRealloc = 0;

		//get the command
		buffer = (char*)malloc(sizeof(char)*BUFFER_SIZE + sizeof(char)); //Don't miss the \0 (+sizeof(char))
		uint32_t i=0;
		char c;

		while(1) //+1 for the \0
		{
			int n = read(STDIN_FILENO, &c, 1);
			if(interrupt)
			{
				interrupt = 0;
				i=0;
				break;
			}

			else if(n == 0)
			{
				printf("\n");
				i = 0;
				exit = 1;
				break;
			}

			else if(c == '\n')
				break;

			buffer[i] = c;
			i++;
			if(i == (nbRealloc+1)*BUFFER_SIZE)
			{
				buffer = (char*)realloc(buffer, (nbRealloc+1)*sizeof(char)*BUFFER_SIZE + sizeof(char)); //Don't miss the \0 (+sizeof(char))
				nbRealloc++;
			}
		}

		if(exit == 1)
		{
			//Free the buffer and exit the program
			free(buffer);
			exit=1;
			// break from while
			break;
		}

		if(i==0)
		{
			free(buffer);
			continue;
		}
		buffer[i] = '\0';

		//Update the history
		history = realloc(history, (historyLen+1)*sizeof(char*));
		history[historyLen] = (char*)malloc((strlen(buffer)+1) * sizeof(char));
		strcpy(history[historyLen], buffer);
		historyLen++;

		//Get the arguments
		char* argv[1024];
		getArgv(buffer, argv);
		if(argv[0] == NULL)
			continue;

		//Then treat them if they are arguments for the shell itself
		//cd
		if(!strcmp(argv[0], "cd"))
		{
			//Only one argument
			if(argv[1] != NULL && argv[2] == NULL)
			{
				if(chdir(argv[1]) == -1)
					printf("Error in cd command\n");
				else
					getcwd(currentDir, BUFFER_SIZE);
			}
			else
				printf("Haven't got the correct number of arguments \n");
		}

		//history
		//the variable history can't be NULL
		else if(!strcmp(argv[0], "history"))
		{
			if(argv[1] == NULL)
			{
				uint32_t i;
				for(i=0; i < historyLen; i++)
					printf("%d\t%s\n", i, history[i]);
			}
		}

		//exit and quit
		else if(!strcmp(argv[0], "exit") || !strcmp(argv[0], "quit"))
		{
			exit = 1;
			break;
		}

		//touch
		else if(!strcmp(argv[0], "touch"))
		{
			//Create the file if needed
			if(access(argv[1], F_OK) == -1)
				creat(argv[1], 644);
			else
				utime(argv[1], NULL);
		}

		//cat
		else if(!strcmp(argv[0], "cat"))
		{
			//Do an echo
			if(argv[1] == NULL)
			{
				char catBuffer[200];
				char c;
				int n;
				uint32_t i=0;
				//Get the character
				while(n = read(STDIN_FILENO, &c, 1))
				{
					if(interrupt || n == 0)
					{
						interrupt = 0;
						break;
					}

					else if(n==-1)
						continue;

					catBuffer[i] = c;
					if(c == '\n')
					{
						catBuffer[i] = '\0';
						printf("%s\n", catBuffer);
						i = 0;
						continue;
					}
					i++;
					//Until we have reach the end of the buffer length
					if(i == 199)
					{
						//Put the end string character
						catBuffer[i] = '\0';
						i=0;
						//and printf the buffer
						printf("%s", catBuffer);
					}
				}
			}

			//print the content of files
			else
			{

			}
		}
		
		//other
		else
		{

			childID = fork();
			if(childID == 0)
			{
				childThread(argv);
			}		

			else
			{
				int wstatus;
				wait(&wstatus);
				childID = 0;
			}
		}

		free(buffer);
	}
	uint32_t i;
	for(i=0; i < historyLen; i++)
		free(history[i]);
	free(history);

	return 0;
}

void handle_int(int num)
{
	if(childID == 0)
	{
		printf("\n");
		interrupt = 1;
	}
}

void getArgv(char* buffer, char** argv)
{
	uint32_t i=0;
	uint32_t argc=0;

	while(buffer[i] != '\0')
	{
		//Cut the buffer
		uint32_t spaceI;
		for(spaceI=0; buffer[i+spaceI] != ' ' && buffer[i+spaceI] != '\0'; spaceI++);

		//Get the argument
		argv[argc] = (char*)malloc(sizeof(char)*spaceI+1);
		uint32_t strCpyI;
		for(strCpyI=0; strCpyI < spaceI; strCpyI++)
			argv[argc][strCpyI] = buffer[i+strCpyI];

		argv[argc][spaceI] = '\0';
		argc++;

		for(spaceI=spaceI; buffer[i+spaceI] == ' '; spaceI++);
		i+=spaceI;
	}
	argv[argc] = NULL;
}

void childThread(char** argv)
{ 
	if(execvp(argv[0], argv) == -1)
		printf("Error %d \n", errno);
}

void getSigInt(int sig)
{
	if(childID != -1)
	{
	}
	fcntl(STDIN_FILENO, F_SETSIG, 0);
}
