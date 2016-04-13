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
#include <pwd.h>

#define BUFFER_SIZE 1023
#define LEN_USER 50
#define LEN_MACHINE 50

pid_t childID=0;
uint32_t interrupt=0;
void childThread(char* path, char** argv);
void split(const char* buffer, char** argv, char splitChar);
void handle_int(int num);

int main()
{
	//Get the current working directory
	char currentDir[BUFFER_SIZE+1];
	getcwd(currentDir, BUFFER_SIZE);
	uint8_t exit=0;

	//Init history
	char** history=NULL; 
	uint32_t historyLen=0;

	//Get the host name and the machine
	char machine[LEN_MACHINE];
	char userName[LEN_USER];
	gethostname(machine, LEN_MACHINE);
	getlogin_r(userName, LEN_USER);

	//Get the home directory
	struct passwd *pw = getpwuid(getuid());
	const char *homedir = pw->pw_dir;
	
	//Define a callback to sigint
	struct sigaction int_handler = {.sa_handler=handle_int};
	int_handler.sa_flags = SA_NOCLDSTOP | SA_NOCLDWAIT;
	sigaction(SIGINT,&int_handler,0);

	while(!exit)
	{
		//Define default pipeline
		FILE* out = stdout;
		FILE* err = stderr;
		FILE* in  = stdin;
	
		char errMode[2] = "w";
		char outMode[2] = "w";

		interrupt = 0;
		//Print the current directory
		printf("%s@%s:%s >", userName, machine, currentDir);
		fflush(stdout);

		//buffer : the command line
		char* buffer=NULL;
		uint32_t nbRealloc = 0; //Number of times we need to realloc the array

		//get the command
		//Update the array size
		buffer = (char*)malloc(sizeof(char)*BUFFER_SIZE + sizeof(char)); //Don't miss the \0 (+sizeof(char))
		uint32_t i=0;
		char c;

		//Get command line from stdin
		while(1) //+1 for the \0
		{
			//Get the next character to stdin and get the error if it exist
			int n = read(STDIN_FILENO, &c, 1);
			//If we have interrupt from the Ctrl+C (SIGINT)
			if(interrupt)
			{
				interrupt = 0;
				i=0;
				break;
			}

			//CTRL+D is pressed 
			else if(n == 0)
			{
				printf("\n");
				i = 0;
				exit = 1;
				break;
			}

			//If the character is a new line
			else if(c == '\n')
				break;

			//and save the character to the buffer
			buffer[i] = c;
			i++;
			//Don't forgot to realloc is needed
			if(i == (nbRealloc+1)*BUFFER_SIZE)
			{
				nbRealloc++;
				buffer = (char*)realloc(buffer, (nbRealloc+1)*sizeof(char)*BUFFER_SIZE + sizeof(char)); //Don't miss the \0 (+sizeof(char))
			}
		}

		//CTRL+D is pressed
		if(exit == 1)
		{
			//Free the buffer and exit the program
			free(buffer);
			exit=1;
			// break from while
			break;
		}

		buffer[i] = '\0';

		//Update the history
		history = realloc(history, (historyLen+1)*sizeof(char*));
		history[historyLen] = (char*)malloc((strlen(buffer)+1) * sizeof(char));
		strcpy(history[historyLen], buffer);
		historyLen++;

		//Get the arguments
		char* argv[1024];
		split(buffer, argv, ' ');
		if(argv[0] == NULL)
			continue;

		
		uint32_t i;
		uint32_t commandCorrect = 1;
		for(i=1; argv[i]; i++)
		{
			if(argv[i][0] == '>')
			{
				//Check if the command is correct
				if(!argv[i+1])
				{
					commandCorrect = 0;
					break;
				}

				else if(argv[i][1] == '\0')
				{
				}

				//Change the mode of the pipe
				else if(argv[i][1] == '>' && argv[i][2] == '\0')
				{
					outMode[0] = 'a';
				}
				
				else
				{
					commandCorrect = 0;
					break;
				}
				//Then open the file (create it if needed)
			}
		}
		if(commandCorrect == 0)
			goto endFor;

		//Then treat them if they are arguments for the shell itself

		//cd: If we change the current directory
		if(!strcmp(argv[0], "cd"))
		{
			if(argv[1] == NULL)
				chdir(homedir);

			else if(argv[1] != NULL && argv[2] == NULL)
			{
				char path[BUFFER_SIZE];
				if(argv[1][0] == '~')
				{
					strcpy(path, homedir);
					strcpy(path+strlen(homedir), &(argv[1][1]));
				}
				else
					strcpy(path, argv[1]);
				printf("path %s argv[1] %s \n", path, argv[1]);
				if(chdir(path) == -1)
					printf("Error in cd command \n");
			}
			
			//Update it
			getcwd(currentDir, BUFFER_SIZE);
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
					//Interrupt has appeared (CTRL + C or CTRL + D)
					if(interrupt || n == 0)
					{
						interrupt = 0;
						break;
					}

					//Error has occured, read the character again
					else if(n==-1)
						continue;

					if(c == '\n')
					{
						catBuffer[i] = '\0';
						printf("%s\n", catBuffer);
						i = 0;
						continue;
					}
					catBuffer[i] = c;
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
				uint32_t i=1;
				for(i=1; argv[i]; i++)
				{
					FILE* f=fopen(argv[i],"r");
					if (f==NULL)
					{
						printf("Erreur in file\n");
						continue;
					}
					char catBuffer[200];
					while(fgets(catBuffer, 200, f) != NULL)
						printf("%s", catBuffer);
					fclose(f);
				}
			}
		}
		
		//Else we execute the command line
		else
		{
			char* programName    = argv[0];
			uint8_t correctProgramName = 1;
			if(access(programName, X_OK) == -1) //If the program isn't in our current directory
			{
				correctProgramName = 0;
				//Split the PATH environment value
				const char* pathEnv = getenv("PATH");
				char* splitPath[256];
				split(pathEnv, splitPath, ':');

				//Test for each subPath
				for(i=0; splitPath[i]; i++)
				{
					char testProgram[1024];
					printf("splitPath[i] %s \n", splitPath[i]);
					strcpy(testProgram, splitPath[i]);
					if(splitPath[i][strlen(splitPath[i])-1] == '/')
						strcpy(testProgram + strlen(splitPath[i]), argv[0]);
					else
					{
						testProgram[strlen(splitPath[i])] = '/';
						strcpy(testProgram + strlen(splitPath[i])+1, argv[0]);
					}
					if(access(testProgram, X_OK) == 0)
					{
						strcpy(programName, testProgram);
						correctProgramName = 1;
						break;
					}
				}
				//free path splited
				for(i=0; splitPath[i]; i++)
					free(splitPath[i]);
			}

			if(!correctProgramName)
			{
				printf("Command not found\n");
			}
			else
			{
				childID = fork();
				//We are the child
				if(childID == 0)
				{
					childThread(programName, argv);
				}		
				//We are the parent
				else
				{
					//Wait for the child to finish
					int wstatus;
					wait(&wstatus);
					childID = 0;
				}
			}
		}

		endFor:
		for(i=0; argv[i]; i++)
			free(argv[i]);

		free(buffer);
	}
	//Free history
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

void split(const char* buffer, char** argv, char splitChar)
{
	uint32_t i=0;
	uint32_t argc=0;

	while(buffer[i] != '\0')
	{
		//Cut the buffer
		uint32_t splitI;
		for(splitI=0; buffer[i+splitI] != splitChar && buffer[i+splitI] != '\0'; splitI++);

		//Get the argument
		argv[argc] = (char*)malloc(sizeof(char)*splitI+1);
		uint32_t strCpyI;
		for(strCpyI=0; strCpyI < splitI; strCpyI++)
			argv[argc][strCpyI] = buffer[i+strCpyI];

		argv[argc][splitI] = '\0';
		argc++;

		for(splitI=splitI; buffer[i+splitI] == splitChar; splitI++);
		i+=splitI;
	}
	argv[argc] = NULL;
}

void childThread(char* path, char** argv)
{ 
	if(execv(argv[0], argv) == -1)
		exit(-1);
}
