#include "touch.h"

void touch(char** argv)
{
	char* fileNames[20];
	uint8_t nbFiles=0;

	uint8_t setAccessTime=1;
	uint8_t setModifTime=1;

	//Get arguments
	uint32_t i=0;
	for(i=1; argv[i]; i++)
	{
		if(!strcmp(argv[i], "-m") && nbFiles == 0)
			setAccessTime=0;
	
		else if(argv[i][0] != '-')
		{
			fileNames[nbFiles] = argv[i];
			nbFiles++;
		}		

		else
		{
			write(err, "Bad arguments", 13);
			return;
		}
	}

	for(i=0; i < nbFiles; i++)
	{
		//Create the file if needed
		if(access(fileNames[i], F_OK) == -1)
			creat(fileNames[i], 0777);

		//Or set its time access
		else
		{
			struct utimbuf b;
			struct stat t;

			stat(fileNames[i], &t);
			if(setModifTime)
				b.modtime = time(NULL);
			else
				b.modtime = t.st_mtime;

			if(setAccessTime)
				b.actime = time(NULL);
			else
				b.actime = t.st_atime;

			utime(fileNames[i], &b);
		}
	}
}
