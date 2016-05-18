#include "copy.h"

int copy(char* source, char* destination)
{
	struct stat sourceStat;
	int error = stat(source, &sourceStat);
	if(error == -1)
	{
		write(err, "Error in getting source file stats. Exit \n", 42);
		return EXIT_FAILURE;
	}

	char freeDestination = 0;
	//Look if the destination is a directory. If it is, the file will be copied in it
	struct stat s;
	int32_t err = stat(destination, &s);
	if(err != -1)
	{
		if(S_ISDIR(s.st_mode))
		{
			freeDestination = 1;
			char* fileName = basename(source);

			uint32_t dLen = strlen(destination);
			uint32_t fLen = strlen(fileName);

			char* dMalloc = (char*)malloc((dLen + 1 + fLen + 1)*sizeof(char));
			memcpy(dMalloc, destination, dLen);
			if(destination[dLen-1] != '/')
			{
				dMalloc[dLen] = '/';
				memcpy(dMalloc+dLen+1, fileName, fLen);
				dMalloc[dLen+1+fLen] = '\0';
			}
			else
			{
				memcpy(dMalloc+dLen, fileName, fLen);
				dMalloc[dLen+fLen] = '\0';
			}
			destination = dMalloc;
		}
	}

	if(S_ISDIR(sourceStat.st_mode))
	{
		int status = copyDir(&sourceStat, source, destination);
		if(status == EXIT_FAILURE)
		{
			if(freeDestination)
				free(destination);
			write(err, "Error while copy \n", 18);
			return EXIT_FAILURE;
		}
	}

	else
	{
		int status = copyFile(&sourceStat, source, destination);
		if(status == EXIT_FAILURE)
		{
			if(freeDestination)
				free(destination);
			write(err, "Error while copy \n", 18);
			return EXIT_FAILURE;
		}
	}
	return 0;
}

int copyFile(struct stat* sourceStat, char* sPath, char* dPath)
{
	int source      = open(sPath, O_RDWR);
	if(source == -1)
	{
		printf("Error while opening the file source %s \n", sPath);
		return EXIT_FAILURE;
	}

	int destination = open(dPath, O_RDWR | O_CREAT, sourceStat->st_mode);
	if(destination == -1)
	{
		printf("Error while creating the file destination %s \n", dPath);
		return EXIT_FAILURE;
	}

	int32_t length=0;
	char buffer[LENGTH];

	while((length = read(source, buffer, LENGTH)) != 0)
	{
		//Error handler
		if(length == -1)
		{
			if(errno == EAGAIN || errno == EINTR)
				continue;
		}

		main_writeAgain://Use for errors
		write(destination, buffer, length);
		if(errno == EAGAIN || errno == EINTR)
			goto main_writeAgain;
	}

	close(source);
	close(destination);
	return EXIT_SUCCESS;
}

int copyDir(struct stat* sourceStat, char* sPath, char* dPath)
{
	struct stat s;
	int32_t err = stat(dPath, &s);
	if(err != -1)
	{
		printf("Error while copying the dir %s. The dir %s already exists \n", sPath, dPath);
		return EXIT_FAILURE;
	}
	char freeSPath = 0;
	char freeDPath = 0;

	uint32_t sLen = strlen(sPath);
	uint32_t dLen = strlen(dPath);

	if(sPath[sLen-1] != '/')
	{
		freeSPath = 1;
		char* sMalloc = (char*)malloc((sLen+2)*sizeof(char));
		memcpy(sMalloc, sPath, sLen);
		sMalloc[sLen] = '/';
		sMalloc[sLen+1] = '\0';
		sPath = sMalloc;
		sLen++;
	}

	if(dPath[dLen-1] != '/')
	{
		freeDPath = 1;
		char* dMalloc = (char*)malloc((dLen+2)*sizeof(char));
		memcpy(dMalloc, dPath, dLen);
		dMalloc[dLen] = '/';
		dMalloc[dLen+1] = '\0';
		dPath = dMalloc;
		dLen++;
	}

	int error = mkdir(dPath, sourceStat->st_mode);
	if(error == -1)
	{
		if(errno != EEXIST)
		{
			printf("Error while copying the directory. Exit \n");
			return EXIT_FAILURE;
		}
	}

	DIR* sDir = opendir(sPath);
	if(sDir == NULL)
	{
		printf("Can't open the dir %s. Abort\n", sPath);
		return EXIT_FAILURE;
	}
	struct dirent* dirent;

	while((dirent = readdir(sDir)) != NULL)
	{
		if(!strcmp(dirent->d_name , ".") || !strcmp(dirent->d_name, ".."))
			continue;

		uint32_t d_nameLen = strlen(dirent->d_name);

		char* sMalloc = (char*)malloc((sLen + d_nameLen + 1)*sizeof(char));
		char* dMalloc = (char*)malloc((dLen + d_nameLen + 1)*sizeof(char));

		memcpy(sMalloc, sPath, sLen);
		memcpy(sMalloc+sLen, dirent->d_name, d_nameLen);
		sMalloc[sLen+d_nameLen] = '\0';

		memcpy(dMalloc, dPath, dLen);
		memcpy(dMalloc+dLen, dirent->d_name, d_nameLen);
		dMalloc[dLen+d_nameLen] = '\0';

		copy(sMalloc, dMalloc);
		free(sMalloc);
		free(dMalloc);
	}

	if(freeSPath)
		free(sPath);
	if(freeDPath)
		free(dPath);
	closedir(sDir);
}
