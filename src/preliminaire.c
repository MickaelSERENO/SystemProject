#include "stdio.h"
#include "stdlib.h"
#include "sys/wait.h"
#include "stdint.h"
#include "string.h"
#include "unistd.h"
#include "errno.h"
#include <pwd.h>

#define BUFFER_SIZE 1023
#define LEN_USER 50
#define LEN_MACHINE 50

void childThread(char** argv);
void getArgv(char* buffer, char** argv);

int main()
{
	//Get the current working directory
	char currentDir[BUFFER_SIZE+1];
	getcwd(currentDir, BUFFER_SIZE);

	//Get the host name and the machine
	char machine[LEN_MACHINE];
	char userName[LEN_USER];
	gethostname(machine, LEN_MACHINE);
	getlogin_r(userName, LEN_USER);

	//Get the home directory
	struct passwd *pw = getpwuid(getuid());
	const char *homedir = pw->pw_dir;

	while(1)
	{

		//Print the current directory
		printf("%s@%s:%s >", userName, machine, currentDir);
		uint8_t exit=0;

		//buffer : the command line
		char* buffer=NULL;
		uint32_t nbRealloc = 0; //Number of times we need to realloc the array

		do
		{
			//Update the array size
			buffer = (char*)realloc(buffer, (nbRealloc+1)*sizeof(char)*BUFFER_SIZE + sizeof(char)); //Don't miss the \0 (+sizeof(char))
			//And get the command line
			if(fgets(buffer + nbRealloc*BUFFER_SIZE, BUFFER_SIZE+1, stdin) == NULL) //+1 for the \0
			{
				//ctrl+D

				free(buffer);
				exit = 1;
				break;
			}
		}
		while(buffer[strlen(buffer)-1] != '\n');

		//Exit if we have close the pipeline stdin
		if(exit == 1)
			break;
		buffer[strlen(buffer)-1] = '\0'; //replace \n by \0

		//split the buffer by spaces
		char* argv[1024];
		getArgv(buffer, argv);

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
				if(chdir(path) == -1)
					printf("Error in cd command \n");
			}
			
			//Update it
			getcwd(currentDir, BUFFER_SIZE);
		}


		else
		{
			//Fork the process
			pid_t childID = fork();
			//If we are the child
			if(childID == 0)
				childThread(argv);

			//Or if we are the parent
			else
			{
				int wstatus;
				wait(&wstatus); //Wait for the child be finished
				free(buffer);
				
				for(i=0; argv[i]; i++)
					free(argv[i]);
			}
		}
	}
	printf("\n");
	return 0;
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
	argv[argc] = NULL;//Last argument is allways NULL
}

void childThread(char** argv)
{ 
	//Exec the command line
	if(execvp(argv[0], argv) == -1)
		exit(-1);
}
