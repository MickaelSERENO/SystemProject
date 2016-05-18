#include "job.h"

void Job_kill(uint32_t jID, int sig)
{
	int i;
	for(i=0; i < jobID[jID].nbChild; i++)
	{
		kill(jobID[jID].childID[i].pid, sig);
	}
}

void handle_int(int num)
{
	uint8_t hasKilled = 0;
	if(hasCurrentChildID)
	{
		Job_kill(currentJobID, SIGINT);
		hasKilled = 1;
	}

	if(!hasKilled)
		interrupt=1;
	printf("\n");
}

void handle_tstp(int num)
{
	if(hasCurrentJobID)
	{
		int i;
		for(i=0; i < jobID[currentJobID].nbChild; i++)
			kill(jobID[currentJobID].childID[i].pid, SIGSTOP);
		jobID[currentJobID].stopped = 1;
		stopped = 1;
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
		if(splitI == 0)
		{
			for(splitI=splitI; buffer[i+splitI] == splitChar; splitI++);
			i+=splitI;
			continue;
		}
		argv[argc] = (char*)malloc(sizeof(char)*splitI+1);
		uint32_t strCpyI;
		for(strCpyI=0; strCpyI < splitI; strCpyI++)
		{
			argv[argc][strCpyI] = buffer[i+strCpyI];
		}

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
		
	close(child->out);
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

	close(child->err);
		
	return NULL;
}

void waitChild(uint32_t jID, uint32_t cID)
{
	currentChildID    = cID;
	currentJobID      = jID;
	hasCurrentChildID = 1;
	hasCurrentJobID   = 1;

	//Need to look for the pipelines
	if(hasPipedStdout)
		pthread_create(&(jobID[jID].childID[cID].stdoutThread), NULL, getStdoutPipe, (void*)(&jobID[jID].childID[cID]));
	
	if(hasPipedStderr)
		pthread_create(&(jobID[jID].childID[cID].stderrThread), NULL, getStderrPipe, (void*)(&jobID[jID].childID[cID]));

	int wstatus;
	waitpid(jobID[jID].childID[cID].pid, &wstatus, WUNTRACED);

	//CTRL+Z done
	if(stopped)
	{
		hasCurrentChildID = 0;
		hasCurrentJobID   = 0;
		printf("[%d] \t Stopped \n", currentJobID);
		stopped=0;
	}

	//The programmed has finished
	else
	{
		//close(jobID[jID].childID[cID].inPipe[1]);
		free(jobID[jID].childID);
		int i;
		for(i=currentJobID; i < (int)nbJob-1; i++)
			jobID[i] = jobID[i+1];
		nbJob--;
		hasCurrentJobID=0;
		hasCurrentChildID=0;
	}
	
}

void childThread(char* path, char** argv)
{ 
	if(execv(argv[0], argv) == -1)
		exit(-1);
}

