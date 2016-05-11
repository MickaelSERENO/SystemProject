#define _GNU_SOURCE
#include "global.h"
#include "cat.h"
#include "cd.h"
#include "Child.h"
#include "pthread.h"

char stdoutFileName[BUFFER_SIZE];
char stderrFileName[BUFFER_SIZE];

//Define default pipeline
int out = 0;
int err = 0;
int in  = 0;

char stdoutMode='w';
char stderrMode='w';

int stdoutPipe[2];
int stderrPipe[2];

uint8_t hasPipedStdout=0;
uint8_t hasPipedStderr=0;

//Define user configuration
char currentDir[BUFFER_SIZE+1];
char machine[LEN_MACHINE];
char userName[LEN_USER];
const char* homedir;

//Define all child launched
Child* childID = NULL;
uint32_t nbChild = 0;
uint32_t currentChildID = 0;
uint8_t hasCurrentChildID=0;

//Define interruption
uint32_t interrupt=0; //CTRL+C
uint32_t stopped=0; //CTRL+Z

void childThread(char* path, char** argv);
void split(const char* buffer, char** argv, char splitChar);
void handle_int(int num);
void handle_tstp(int num);
void* getStdoutPipe(void* pipe);
void* getStderrPipe(void* pipe);
void waitChild(uint32_t cID);

int main()
{
	//Get the current working directory
	getcwd(currentDir, BUFFER_SIZE);
	uint8_t exit=0;

	//Init history
	char** history=NULL; 
	uint32_t historyLen=0;

	//Get the host name and the machine
	gethostname(machine, LEN_MACHINE);
	getlogin_r(userName, LEN_USER);

	//Get the home directory
	struct passwd *pw = getpwuid(getuid());
	homedir = pw->pw_dir;
	
	//Define a callback to sigint (CTRL + C)
	struct sigaction int_handler = {.sa_handler=handle_int};
	int_handler.sa_flags = SA_NOCLDSTOP | SA_NOCLDWAIT;
	sigaction(SIGINT,&int_handler,0);

	//Define a callback to sigtstp (CTRL + Z)
	struct sigaction tstp_handler = {.sa_handler=handle_tstp};
	tstp_handler.sa_flags = SA_NOCLDSTOP | SA_NOCLDWAIT;
	sigaction(SIGTSTP,&tstp_handler, 0);
	
	childID = (Child*)malloc(10*sizeof(Child));

	while(!exit)
	{
		//Reset the default pipeline
		out = STDOUT_FILENO;
		err = STDERR_FILENO;
		in = STDIN_FILENO;	

		hasPipedStdout=0;
		hasPipedStderr=0;
		
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
			//fileno : get the fd of a FILE*
			int n = read(in, &c, 1);
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

		uint32_t commandCorrect = 1;
		for(i=1; argv[i]; i++)
		{
			uint32_t j=0;
			if(!commandCorrect)
				break;
			uint8_t willAppend=0;
			for(j=0; argv[i][j]; j++)
			{
				if(argv[i][j] == '>')
				{
					if(argv[i][j+1] == '>')
						willAppend=1;
						
					//Check if the command is correct
					if(!argv[i][j+1] || (willAppend && !argv[i][j+2]))
						commandCorrect = 0;

					//Redirect stdout
					else if(j==0 || argv[i][j-1] == '1')
					{
						//Get the fileName which will be now the new stdout
						if(willAppend)
						{
							strcpy(stdoutFileName, &(argv[i][j+2]));
							stdoutMode = 'a';
						}
						else
							strcpy(stdoutFileName, &(argv[i][j+1]));
						pipe(stdoutPipe);
						hasPipedStdout=1;
					}
					
					//Redirect stderr
					else if(argv[i][j-1] == '2')
					{
						if(willAppend)
						{
							strcpy(stderrFileName, &(argv[i][j+2]));
							stdoutMode = 'a';
						}
						else
							strcpy(stderrFileName, &(argv[i][j+1]));
						pipe(stderrPipe);
						hasPipedStderr=1;
					}
				
					else
						commandCorrect = 0;
					
					free(argv[i]);
					argv[i] = NULL;
					break;
				}
			}
		}
		if(commandCorrect == 0)
			goto endFor;
			
			//Open it
			if(hasPipedStderr)
			{
				//Create this file if doesn't exist
				if(access(stderrFileName, F_OK) == -1)
					creat(stderrFileName, 0777);

				if(stderrMode == 'a')
					err=open(stderrFileName, O_WRONLY | O_APPEND);
				else
					err=open(stderrFileName, O_WRONLY);
			}
			
			else if(hasPipedStdout)
			{
				//Create this file if doesn't exist
				if(access(stdoutFileName, F_OK) == -1)
					creat(stdoutFileName, 0666);

				if(stdoutMode == 'a')
					out = open(stdoutFileName, O_WRONLY | O_APPEND);
				else
					out = open(stdoutFileName, O_WRONLY);

			}
		//Then treat them if they are arguments for the shell itself
		//cd: If we change the current directory
		if(!strcmp(argv[0], "cd"))
			cd(argv);

		//history
		//the variable history can't be NULL
		else if(!strcmp(argv[0], "history"))
		{
			if(argv[1] == NULL)
			{
				uint32_t i;
				for(i=0; i < historyLen; i++)
					write(out, history[i], strlen(history[i]));
			}
		}

		//exit and quit
		else if(!strcmp(argv[0], "exit") || !strcmp(argv[0], "quit"))
		{
			exit = 1;
			break;
		}

		//fg command
		else if(!strcmp(argv[0], "fg") && argv[1] != NULL && argv[2] == NULL)
		{
			uint32_t id = atoi(argv[1]);
			currentChildID = id;
			kill(childID[id].pid, SIGCONT);
			waitChild(id);
		}

		//bg command
		else if(!strcmp(argv[0], "bg") && argv[1] != NULL && argv[2] == NULL)
		{
			uint32_t id = atoi(argv[1]);
			kill(childID[id].pid, SIGCONT);
			//Don't transmit CTRL+C
			setpgid(childID[id].pid, 0);
		}

		//touch
		else if(!strcmp(argv[0], "touch"))
			touch(argv);

		//cat
		else if(!strcmp(argv[0], "cat"))
			cat(argv);
		
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
					//Get the full path
					char testProgram[1024];
					strcpy(testProgram, splitPath[i]);
					if(splitPath[i][strlen(splitPath[i])-1] == '/')
						strcpy(testProgram + strlen(splitPath[i]), argv[0]);
					else
					{
						testProgram[strlen(splitPath[i])] = '/';
						strcpy(testProgram + strlen(splitPath[i])+1, argv[0]);
					}
					
					//Then we look if the path gives a file executable
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
				write(err, "Command not found\n", 18);
			}
			
			else
			{
				pid_t pid = fork();
				//We are the child
				if(pid == 0)
				{
					if(hasPipedStdout)
					{
						dup2(stdoutPipe[1], STDOUT_FILENO);
						close(stdoutPipe[1]);
					}
					
					if(hasPipedStderr)
					{
						dup2(stderrPipe[1], STDERR_FILENO);
						close(stderrPipe[1]);
					}
					
					setsid();
					setpgid(pid, 0);
					childThread(programName, argv);
				}		
				//We are the parent
				else
				{
					//Save this child id
					childID[nbChild].pid     = pid;
					childID[nbChild].stopped = 0;
					childID[nbChild].errPipe[0] = stderrPipe[0];
					childID[nbChild].errPipe[1] = stderrPipe[2];

					childID[nbChild].outPipe[0] = stdoutPipe[0];
					childID[nbChild].outPipe[1] = stdoutPipe[2];

					childID[nbChild].out = out;
					childID[nbChild].err = err;

					nbChild++;

					//Remember that our array has a max size. Need to be larger
					if(nbChild % 10 == 0)
						childID = (Child*)realloc(childID, (10+nbChild)*sizeof(Child));

					waitChild(nbChild-1);	
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
	uint8_t hasKilled = 0;
	if(hasCurrentChildID)
	{
		kill(childID[currentChildID].pid, SIGINT);
		hasKilled = 1;
	}
	if(!hasKilled)
		interrupt=1;
	printf("\n");
}

void handle_tstp(int num)
{
	if(nbChild != 0)
	{
		if(hasCurrentChildID)
		{
			printf("currentChildID %d pid %d \n", currentChildID, childID[currentChildID].pid);
			kill(childID[currentChildID].pid, SIGSTOP);
			childID[currentChildID].stopped=1;
			stopped = 1;
		}
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

void* getStdoutPipe(void* data)
{
	Child *child = (Child*)data;
	int fd = child->outPipe[0];
							
	//And write the incoming of the stdout
	char c;
	while(read(fd, &c, 1)!=0)
		write(child->out, &c, sizeof(char));
		
	return NULL;
}

void* getStderrPipe(void* data)
{
	Child *child = (Child*)data;
	int fd = child->errPipe[0];
							
	//And write the incoming of the stdout
	char c;
	while(read(fd, &c, 1)!=0)
		write(child->err, &c, sizeof(char));
		
	return NULL;
}

void waitChild(uint32_t cID)
{
	currentChildID = cID;
	hasCurrentChildID = 1;

	//Need to look for the pipelines
	if(hasPipedStdout)
		pthread_create(&(childID[cID].stdoutThread), NULL, getStdoutPipe, (void*)(&childID[cID]));
	
	if(hasPipedStderr)
		pthread_create(&(childID[cID].stderrThread), NULL, getStderrPipe, (void*)(&childID[cID]));

	int wstatus;
	waitpid(childID[cID].pid, &wstatus, WUNTRACED);

	//CTRL+Z done
	if(stopped)
	{
		hasCurrentChildID = 0;
		printf("[%d] \t Stopped \n", currentChildID);
		stopped=0;
	}

	//The programmed has finished
	else
	{
		nbChild--;
		uint32_t i;
		for(i=currentChildID; i < nbChild; i++)
			childID[i] = childID[i+1];
	}
}

void childThread(char* path, char** argv)
{ 
	if(execv(argv[0], argv) == -1)
		exit(-1);
}
