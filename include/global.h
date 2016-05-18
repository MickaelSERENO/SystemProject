#ifndef  GLOBAL_INC
#define  GLOBAL_INC

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
#include "Child.h"

#define BUFFER_SIZE 2047
#define LEN_USER 50
#define LEN_MACHINE 50

typedef struct Job
{
	Child* childID;
	uint32_t nbChild;
	uint8_t stopped;
	char command[2048];
}Job;

//stream to input, error and output
extern int in;
extern int err;
extern int out;

extern char stdoutFileName[BUFFER_SIZE];
extern char stderrFileName[BUFFER_SIZE];

extern int stdoutPipe[2];
extern int stderrPipe[2];
extern int stdinPipe[2];

extern uint8_t hasPipedStdout;
extern uint8_t hasPipedStderr;
extern uint8_t hasPipedStdin;

extern char stdoutMode;
extern char stderrMode;

//User settings
extern char currentDir[BUFFER_SIZE+1];
extern char machine[LEN_MACHINE];
extern char userName[LEN_USER];
extern const char* homedir;

//Init history
extern char** history; 
extern uint32_t historyLen;

//Use if the interruption SIGINT was caught
extern uint32_t interrupt;
extern uint32_t stopped;

extern uint8_t quit;
extern Job* jobID;
extern uint32_t nbJob;
extern uint32_t currentChildID;
extern uint8_t hasCurrentChildID;
extern uint32_t currentJobID;
extern uint8_t hasCurrentJobID;

#endif
