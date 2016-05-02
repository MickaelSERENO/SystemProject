#include "touch.h"

void touch(char** argv)
{
	//Create the file if needed
	if(access(argv[1], F_OK) == -1)
		creat(argv[1], 644);
	//Or set its time access
	else
		utime(argv[1], NULL);
}
