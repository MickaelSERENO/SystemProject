#ifndef  COPY_INC
#define  COPY_INC

#include "stdio.h"
#include "libgen.h"
#include "stdlib.h"
#include "unistd.h"
#include "errno.h"
#include "sys/types.h"
#include "stdint.h"
#include "sys/stat.h"
#include "fcntl.h"
#include "dirent.h"
#include "string.h"

#define LENGTH 512


#include "global.h"

int copyFile(struct stat* sourceStat, char* sPath, char* dPath);
int copyDir(struct stat* sourceStat, char* sPath, char* dPath);
int copy(char* source, char* destination);

#endif
