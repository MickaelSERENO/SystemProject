#ifndef  CHILD_INC
#define  CHILD_INC

#include "pthread.h"

typedef struct Child
{
	uint32_t pid;
	int outPipe[2];
	int errPipe[2];
	int out;
	int err;
	uint8_t stopped;
	pthread_t stdoutThread;
	pthread_t stderrThread;
}Child;

#endif
