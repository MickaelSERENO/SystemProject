#ifndef  CAT_INC
#define  CAT_INC

#include "stdio.h"
#include "stdlib.h"
#include "global.h"
#include "stdint.h"
#include "string.h"
#include "unistd.h"

typedef struct Cat
{
	uint8_t showLines;
}Cat;

void cat(char** argv);
void catEcho(Cat* c);
void catFile(Cat* c, const char* fileName);

#endif
