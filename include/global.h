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

#define BUFFER_SIZE 1023
#define LEN_USER 50
#define LEN_MACHINE 50


//stream to input, error and output
extern FILE* in;
extern FILE* err;
extern FILE* out;

//User settings
extern char currentDir[BUFFER_SIZE+1];
extern char machine[LEN_MACHINE];
extern char userName[LEN_USER];
extern const char* homedir;

//Use if the interruption SIGINT was caught
uint32_t interrupt;

#endif
