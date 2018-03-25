#include <stdlib.h>
#include <stdio.h>
int main(int argc, char const *argv[])
{
	wchar_t s;
	if (argc > 1)
	{
		s = atoi(argv[1]);
		if (s == 0)
		{
			printf("Incorrect argument!\n");
			exit(EXIT_FAILURE);
		}
	}
	else s = 2048;
	printf("Preparing to reserve %ld kilobytes of space.\n", s*1024*sizeof(char));
	char *tab = calloc(s * 1024, sizeof(char)); 

	if (tab == NULL)
	{
		printf("Failure to reserve space.\n");
		exit(EXIT_FAILURE);
	}
	printf("Memory space successfully allocated.\n");
	exit(EXIT_SUCCESS);
}