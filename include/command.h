#ifndef  COMMAND_INC
#define  COMMAND_INC

#include "global.h"
#include "job.h"
#include "copy.h"
#include "cat.h"
#include "touch.h"
#include "cd.h"

void execCommand(char* command, uint8_t endJob);

#endif
