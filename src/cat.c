#include "cat.h"

void cat(char** argv)
{
	//Do an echo
	if(argv[1] == NULL)
	{
		char catBuffer[200];
		char c;
		int n;
		uint32_t i=0;

		//Get the character
		while(n = read(STDIN_FILENO, &c, 1))
		{
			//Interrupt has appeared (CTRL + C or CTRL + D)
			if(interrupt || n == 0)
			{
				interrupt = 0;
				break;
			}

			//Error has occured, read the character again
			else if(n==-1)
				continue;

			if(c == '\n')
			{
				catBuffer[i] = '\0';
				write(out, catBuffer, strlen(catBuffer));
				write(out, "\n", 1);
				i = 0;
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

	//print the content of files
	else
	{
		uint32_t i=1;
		for(i=1; argv[i]; i++)
		{
			FILE* f=fopen(argv[i],"r");
			if (f==NULL)
			{
				write(err, "Erreur in file\n", 15);
				continue;
			}
			char catBuffer[200];
			while(fgets(catBuffer, 200, f) != NULL)
				write(out, catBuffer, strlen(catBuffer));
			fclose(f);
		}
	}
}
