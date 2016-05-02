#include "cd.h"

void cd(char** argv)
{
	if(argv[1] == NULL)
		chdir(homedir);

	else if(argv[1] != NULL && argv[2] == NULL)
	{
		char path[BUFFER_SIZE];
		if(argv[1][0] == '~')
		{
			strcpy(path, homedir);
			strcpy(path+strlen(homedir), &(argv[1][1]));
		}
		else
			strcpy(path, argv[1]);
		printf("path %s argv[1] %s \n", path, argv[1]);
		if(chdir(path) == -1)
			printf("Error in cd command \n");
	}
	
	//Update it
	getcwd(currentDir, BUFFER_SIZE);

}
