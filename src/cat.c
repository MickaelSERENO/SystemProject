#include "cat.h"

void cat(char** argv)
{
	Cat c = {.showLines = 0};
	
	//List all file names
	//NULL is for stdin
	const char* fileNames[10];
	uint8_t nbFiles     = 0;

	uint32_t i;
	for(i=1; argv[i] != NULL; i++)
	{
		//Arguments
		if(!strcmp(argv[i], "-n"))
		{
			if(nbFiles != 0)
			{
				write(err, "Bad arguments", 13);
				return;
			}

			c.showLines = 1;
		}

		//File name
		else if(argv[i][0] != '-')
		{
			fileNames[nbFiles] = argv[i];
			nbFiles++;
		}

		//Do an echo
		else if(argv[i][0] == '-' && argv[i][1] == '\0')
		{
			fileNames[nbFiles] = NULL;
			nbFiles++;
		}

		else
		{
			write(err, "Bad arguments", 13);
			return;
		}
	}

	//No files --> do an echo
	if(nbFiles == 0)
	{
		catEcho(&c);

		//CTRL+C happened
		if(interrupt)
		{
			interrupt = 0;
			return;
		}
	}
	else
	{
		for(i=0; i < nbFiles; i++)
		{
			if(fileNames[i] == NULL)
				catEcho(&c);
			else
				catFile(&c, fileNames[i]);

			//CTRL+C happened
			if(interrupt)
			{
				interrupt = 0;
				return;
			}
		}
	}
}

void catEcho(Cat* ca)
{
	char catBuffer[200];
	char c;
	int n;
	uint32_t i=0;
	uint32_t lineID = 1;
	uint8_t hasPrintNb = 0;

	//Get the character
	while(n = read(STDIN_FILENO, &c, 1))
	{
		//Interrupt has appeared (CTRL + C or CTRL + D)
		if(interrupt || n == 0)
			break;

		//Error has occured, read the character again
		else if(n==-1)
			continue;

		//Print line id if necessary
		if(ca->showLines && !hasPrintNb)
		{
			char line[1024];
			sprintf(line, "%d", lineID);
			write(out, "\t", 1);
			write(out, line, strlen(line));
			write(out, " ", 1);
			hasPrintNb = 1;			
			lineID++;
		}
		
		if(c == '\n')
		{
			catBuffer[i] = '\0';
			write(out, catBuffer, strlen(catBuffer));
			write(out, "\n", 1);
			i = 0;
			hasPrintNb = 0;
			continue;
		}
		
		catBuffer[i] = c;
		i++;
		//Until we have reach the end of the buffer length
		if(i == 199)
		{
			//Put the end string character
			catBuffer[i] = '\0';
			i=0;
			//and printf the buffer
			write(out, catBuffer, strlen(catBuffer));
		}
	}
}

void catFile(Cat* ca, const char* fileName)
{
	FILE* f=fopen(fileName,"r");
	if (f==NULL)
	{
		write(err, "Erreur in file\n", 15);
		interrupt=1;		
		return;
	}

	char catBuffer[200];
	uint8_t hasPrintNb=0;
	uint32_t lineID=1;

	while(fgets(catBuffer, 200, f) != NULL)
	{
		if(ca->showLines && !hasPrintNb)
		{
			char line[1024];
			sprintf(line, "%d", lineID);
			write(out, line, strlen(line));
			write(out, " ", 1);
			hasPrintNb = 1;			
			lineID++;
		}
		write(out, catBuffer, strlen(catBuffer));
		if(catBuffer[strlen(catBuffer)-1] == '\n')
			hasPrintNb=0;
	}	
	fclose(f);
}
