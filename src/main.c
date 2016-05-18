#define _GNU_SOURCE
#include "touch.h"
#include "cat.h"
#include "cd.h"
#include "Child.h"
#include "pthread.h"
#include "global.h"
#include "job.h"

//Define default pipeline
int out = 0;
int err = 0;
int in  = 0;

int stdoutPipe[2];
int stderrPipe[2];
int stdinPipe[2];

char stdoutFileName[BUFFER_SIZE];
char stderrFileName[BUFFER_SIZE];

char stdoutMode='w';
char stderrMode='w';

uint8_t hasPipedStdout=0;
uint8_t hasPipedStderr=0;
uint8_t hasPipedStdin=0;

//Define user configuration
char currentDir[BUFFER_SIZE+1];
char machine[LEN_MACHINE];
char userName[LEN_USER];
const char* homedir;

//Define all child launched
uint32_t currentChildID = 0;
uint8_t hasCurrentChildID=0;
uint32_t currentJobID=0;
uint8_t hasCurrentJobID=0;

Job*  jobID=NULL;
uint32_t nbJob=0;

//Define interruption
uint32_t interrupt=0; //CTRL+C
uint32_t stopped=0; //CTRL+Z

char** history=NULL; 
uint32_t historyLen=0;

uint8_t quit=0;

int main()
{
	//Get the current working directory
	getcwd(currentDir, BUFFER_SIZE);

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
	
	jobID = (Job*)malloc(10*sizeof(Job));
	nbJob=0;

	while(!quit)
	{
		//Reset the default pipeline
		out = STDOUT_FILENO;
		err = STDERR_FILENO;
		in = STDIN_FILENO;	

		hasPipedStdout=0;
		hasPipedStderr=0;
		hasPipedStdin=0;
		
		interrupt = 0;

		//Print the current directory
		printf("%s@%s:%s >", userName, machine, currentDir);
		fflush(stdout);

		//buffer : the command line
		char* buffer=NULL;
		uint32_t nbRealloc = 0; //Number of times we need to realloc the array

		//Update the array size
		buffer = (char*)malloc(sizeof(char)*BUFFER_SIZE + sizeof(char)); //Don't miss the \0 (+sizeof(char))

		uint32_t i=0;
		char c;

		//Get command line from stdin
		while(!quit)
		{
			//Get the next character to stdin and get the error if it exist
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
				quit = 1;
				break;
			}

			//If the character is a new line
			else if(c == '\n')
				break;

			//and save the character in the buffer
			buffer[i] = c;
			i++;
			//Don't forgot to realloc if needed
			if(i == (nbRealloc+1)*BUFFER_SIZE)
			{
				nbRealloc++;
				buffer = (char*)realloc(buffer, (nbRealloc+1)*sizeof(char)*BUFFER_SIZE + sizeof(char)); //Don't miss the \0 (+sizeof(char))
			}
		}

		//CTRL+D is pressed
		if(quit == 1)
		{
			//Free the buffer and quit the program
			free(buffer);
			quit=1;
			// break from while
			break;
		}

		buffer[i] = '\0';

		//Update the history
		history = realloc(history, (historyLen+1)*sizeof(char*));
		history[historyLen] = (char*)malloc((strlen(buffer)+1) * sizeof(char));
		strcpy(history[historyLen], buffer);
		historyLen++;

		char* jobCommand[1024];
		split(buffer, jobCommand, '|');
		sprintf(jobID[nbJob].command, "%s", buffer);
		jobID[nbJob].childID = (Child*)malloc(10*sizeof(Child));
		jobID[nbJob].nbChild = 0;

		nbJob++;
		if(nbJob % 10 == 0)
			jobID = realloc(jobID, (10+nbJob)*sizeof(Job));

		for(i=0; jobCommand[i]; i++)
		{
			execCommand(jobCommand[i], jobCommand[i+1] == NULL);
			free(jobCommand[i]);
		}
	}

	//Free history
	uint32_t i;
	for(i=0; i < historyLen; i++)
		free(history[i]);
	free(history);

	return 0;
}
