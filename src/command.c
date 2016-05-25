#include "command.h"

void execCommand(char* command, uint8_t endJob)
{
	out = STDOUT_FILENO;
	in = STDIN_FILENO;
	err = STDERR_FILENO;
	int inPipe[2];
	//Get the arguments
	char* argv[1024];
	split(command, argv, ' ');
	if(argv[0] == NULL)
		return;

	//Define redirection
	uint32_t commandCorrect = 1;
	uint32_t i;
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
					{
						strcpy(stdoutFileName, &(argv[i][j+1]));
						stdoutMode = 'w';
					}
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
					{
						strcpy(stderrFileName, &(argv[i][j+1]));
						stderrMode = 'w';
					}
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

	//Stop this command
	if(commandCorrect == 0)
		goto EndFor;
		
	//Open pipelines if needed
	if(hasPipedStderr)
	{
		//Create this file if doesn't exist
		if(access(stderrFileName, F_OK) == -1)
			creat(stderrFileName, 0664);

		if(stderrMode == 'a')
			err=open(stderrFileName, O_WRONLY | O_APPEND);
		else
			err=open(stderrFileName, O_WRONLY);
	}
	
	if(hasPipedStdout)
	{
		//Create this file if doesn't exist
		if(access(stdoutFileName, F_OK) == -1)
			creat(stdoutFileName, 0666);

		if(stdoutMode == 'a')
			out = open(stdoutFileName, O_WRONLY | O_APPEND);
		else
			out = open(stdoutFileName, O_WRONLY);

	}

	if(hasPipedStdin)
		in = stdinPipe[0];

	if(!endJob)
	{
		pipe(inPipe);
		out = inPipe[1];
	}

	int parent = 0;

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
			{
				write(out, history[i], strlen(history[i]));
				write(out, "\n", 1);
			}
		}
	}

	//exit and quit
	else if(!strcmp(argv[0], "exit") || !strcmp(argv[0], "quit"))
	{
		quit = 1;
		goto EndFor;
	}

	//fg command
	else if(!strcmp(argv[0], "fg") && argv[1] != NULL && argv[2] == NULL)
	{
		uint32_t id = atoi(argv[1]);
		if(id < nbJob)
		{
			currentChildID = id;
			Job_kill(id, SIGCONT);
			waitChild(id, jobID[id].nbChild-1);
		}
	}

	//bg command
	else if(!strcmp(argv[0], "bg") && argv[1] != NULL && argv[2] == NULL)
	{
		uint32_t id = atoi(argv[1]);
		if(id < nbJob)
		{
			Job_kill(id, SIGCONT);
			jobID[id].stopped=0;
		}
	}

	//kill command
	else if(!strcmp(argv[0], "kill"))
	{
		uint32_t i;
		for(i=1; argv[i]; i++)
			kill(atoi(argv[i]), SIGINT);
	}

	else if(!strcmp(argv[0], "jobs"))
	{
		uint32_t i;
		for(i=0; i < nbJob; i++)
		{
			char line[1024];
			sprintf(line, "%d", i);
			write(out, "[", 1);
			write(out, line, strlen(line));
			write(out, "]", 1);
			
			write(out, "\t", 1);
			write(out, "[", 1);

			if(jobID[i].stopped)
				write(out, "STOPPED", 7);
			else
				write(out, "RUNNING", 7);
			write(out, "]", 1);

			write(out, "\t\t", 2);
			write(out, jobID[i].command, strlen(jobID[i].command));
			write(out, "\n", 1);
		}
	}

	//touch
	else if(!strcmp(argv[0], "touch"))
		touch(argv);

	//cat
	else if(!strcmp(argv[0], "cat"))
		cat(argv);

	//copy
	else if(!strcmp(argv[0], "cp") && argv[1] != NULL && argv[2] != NULL)
		copy(argv[1], argv[2]);

	else if(!strcmp(argv[0], "wait"))
	{
		for(int i=1; argv[i]; i++)
		{
			int pid = atoi(argv[i]);
			int wstatus;
			waitpid(pid, &wstatus, WUNTRACED);
		}

		if(argv[1] == NULL)
		{
			int wstatus;
			wait(&wstatus);
		}
	}
	
	//Else we execute the command line
	else
	{
		//Get the path of the program called
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

				if(hasPipedStdin)
				{
					dup2(stdinPipe[0], STDIN_FILENO);
					close(stdinPipe[0]);
				}

				if(!endJob)
				{
					dup2(inPipe[1], STDOUT_FILENO);
					close(inPipe[1]);
				}

				//Use for the CTRL+C
				setsid();
				setpgid(pid, 0);

				//Execute the child
				childThread(programName, argv);
			}		

			//We are the parent
			else
			{
				parent = 1;
				//Save this child id
				jobID[nbJob-1].childID[jobID[nbJob-1].nbChild].pid     = pid;
				jobID[nbJob-1].childID[jobID[nbJob-1].nbChild].stopped = 0;
				jobID[nbJob-1].childID[jobID[nbJob-1].nbChild].errPipe[0] = stderrPipe[0];
				jobID[nbJob-1].childID[jobID[nbJob-1].nbChild].errPipe[1] = stderrPipe[1];

				jobID[nbJob-1].childID[jobID[nbJob-1].nbChild].outPipe[0] = stdoutPipe[0];
				jobID[nbJob-1].childID[jobID[nbJob-1].nbChild].outPipe[1] = stdoutPipe[1];

				jobID[nbJob-1].childID[jobID[nbJob-1].nbChild].inPipe[0] = stdinPipe[0];
				jobID[nbJob-1].childID[jobID[nbJob-1].nbChild].inPipe[1] = stdinPipe[1];

				jobID[nbJob-1].childID[jobID[nbJob-1].nbChild].out = out;
				jobID[nbJob-1].childID[jobID[nbJob-1].nbChild].err = err;

				strcpy(jobID[nbJob-1].childID[jobID[nbJob-1].nbChild].command, command);

				jobID[nbJob-1].nbChild++;

				//Remember that our array has a max size. Need to be larger
				if(jobID[nbJob-1].nbChild % 10 == 0)
					jobID[nbJob-1].childID = (Child*)realloc(jobID[nbJob-1].childID, (10+jobID[nbJob-1].nbChild)*sizeof(Child));

				if(endJob)
					waitChild(nbJob-1, jobID[nbJob-1].nbChild-1);	
			}

		}
	}
	if(endJob && !parent)
	{
		nbJob--;
	}

	hasPipedStdin = 1;
	for(i=0; i < 2; i++)
		stdinPipe[i] = inPipe[i];

	EndFor:
	if(endJob && !commandCorrect)
		nbJob--;
	for(i=0; argv[i]; i++)
		free(argv[i]);

	if(!endJob)
		close(inPipe[1]);
}
