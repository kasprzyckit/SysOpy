#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

#define times_start getrusage(RUSAGE_SELF, start); \
					gettimeofday(rstart, NULL);
#define times_end gettimeofday(rend, NULL); \
					getrusage(RUSAGE_SELF, end); \
					print_times(start, end, rstart, rend);

typedef struct sfile {

	int sys;
	FILE *lib;

} sfile;

void print_time(const char* mes, time_t secs, suseconds_t msecs)
{
	if (msecs < 0)
	{
		secs -= 1;
		msecs += 1000000;
	}
	printf("%s: %ld second(s) %ld microsecond(s)\n", mes, secs, msecs);
}

void print_times(struct rusage *start, struct rusage *end, struct timeval *rstart, struct timeval *rend)
{
	print_time("real time", rend->tv_sec - rstart->tv_sec, rend->tv_usec - rstart->tv_usec);
	print_time("user time", end->ru_utime.tv_sec - start->ru_utime.tv_sec, end->ru_utime.tv_usec - start->ru_utime.tv_usec);
	print_time("system time", end->ru_stime.tv_sec - start->ru_stime.tv_sec, end->ru_stime.tv_usec - start->ru_stime.tv_usec);
}

int compare(char* str1, char* str2)
{
	return (((unsigned int)str1[0]) >= ((unsigned int)str2[0]));
}

int sys_read(sfile* file_wrapper, char* ptr, int size, int offset)
{
	if (lseek(file_wrapper->sys, size*offset, SEEK_SET) >= 0)
	{
		return read(file_wrapper->sys, ptr, size);
	}
	else return -1;
}
int sys_write(sfile* file_wrapper, char* ptr, int size, int offset)
{
	if (lseek(file_wrapper->sys, size*offset, SEEK_SET) >= 0)
	{
		return write(file_wrapper->sys, ptr, size);
	}
	else return -1;
}
int lib_read(sfile* file_wrapper, char* ptr, int size, int offset)
{
	if (fseek(file_wrapper->lib, size*offset, 0) == 0)
	{
		return fread(ptr, size, 1, file_wrapper->lib);
	}
	else return -1;
}
int lib_write(sfile* file_wrapper, char* ptr, int size, int offset)
{
	if (fseek(file_wrapper->lib, size*offset, 0) == 0)
	{
		return fwrite(ptr, size, 1, file_wrapper->lib);
	}
	else return -1;
}

void insertion_sort(sfile* file_wrapper, int tab_size, int tab_width, int (*read_function)(sfile*, char*, int, int), int (*write_function)(sfile*, char*, int, int))
{
	char *ptr1 = malloc(tab_width);
	char *ptr2 = malloc(tab_width);
	int i, j;
	for (i = 1; i < tab_size; i++)
	{
		(*read_function)(file_wrapper, ptr1, tab_width, i);
		for (j = i - 1; j >= 0; j--)
		{
			(*read_function)(file_wrapper, ptr2, tab_width, j);
			if (compare(ptr1, ptr2) == 0)
			{
				(*write_function)(file_wrapper, ptr2, tab_width, j+1);
				if (j == 0) (*write_function)(file_wrapper, ptr1, tab_width, 0);
			}
			else
			{
				(*write_function)(file_wrapper, ptr1, tab_width, j+1);
				break;
			}
		}
	}

	free(ptr1);
	free(ptr2);
}

int main(int argc, char const *argv[])
{
	if (argc < 5)
	{
		printf("Too few arguments!\n");
		return 1;
	}

	struct rusage *start = malloc(sizeof(struct rusage));
	struct rusage *end = malloc(sizeof(struct rusage));
	struct timeval *rstart = malloc(sizeof(struct timeval));
	struct timeval *rend = malloc(sizeof(struct timeval));

	int i;
	int tab_size, tab_width, count;
	char *ptr;

	if (strcmp(argv[1], "generate") == 0)
	{
		tab_size = atoi(argv[3]);
		tab_width = atoi(argv[4]);
		FILE *file = fopen(argv[2], "w");
		FILE *gen = fopen("/dev/urandom", "r");
		if (file && gen && tab_size != 0 && tab_width != 0)
		{
			ptr = malloc(tab_width);
			for (i = 0; i<tab_size; i++)
			{
				fread(ptr, 1, tab_width, gen);
				fwrite(ptr, 1, tab_width, file);
			}
			free(ptr);
		}
		else printf("Incorrect arguments!\n");
		if (file) fclose(file);
		if (gen) fclose(gen);

	}
	else if (strcmp(argv[1], "sort") == 0)
	{
		if (argc < 6 || (strcmp(argv[5],"sys")!=0 && strcmp(argv[5],"lib")!=0))
			printf("Incorrect arguments!\n");
		else
		{
			tab_size = atoi(argv[3]);
			tab_width = atoi(argv[4]);
			if (tab_size != 0 && tab_width != 0)
			{
				sfile *file_wrapper = malloc(sizeof(sfile));
				printf("Sorting %s using %s: %i records of %i bytes\n", argv[2], argv[5], tab_size, tab_width);
				if (strcmp(argv[5],"sys") == 0)
				{
					int tab = open(argv[2], O_RDWR);
					if (tab >= 0)
					{
						file_wrapper->sys = tab;
						times_start
						insertion_sort(file_wrapper, tab_size, tab_width, &sys_read, &sys_write);
						times_end
						close(tab);
					}
				}
				else
				{
					FILE *ftab = fopen(argv[2], "r+");
					if (ftab)
					{
						file_wrapper->lib = ftab;
						times_start
						insertion_sort(file_wrapper, tab_size, tab_width, &lib_read, &lib_write);
						times_end
						fclose(ftab);
					}
				}
				free(file_wrapper);
			}
			else printf("Incorrect arguments!\n");
		}
	}
	else if (strcmp(argv[1], "copy") == 0)
	{
		if (argc < 7 || (strcmp(argv[6],"sys")!=0 && strcmp(argv[6],"lib")!=0))
			printf("Incorrect arguments!\n");
		else
		{
			tab_size = atoi(argv[4]);
			tab_width = atoi(argv[5]);
			if (tab_size != 0 && tab_width != 0)
			{
				ptr = malloc(tab_width);
				printf("Copying %s to %s using %s: %i records of %i bytes\n", argv[2], argv[3], argv[6], tab_size, tab_width);
				if (strcmp(argv[6],"sys") == 0)
				{
					int in, out;
					in = open(argv[2], O_RDONLY);
					out = open(argv[3], O_WRONLY|O_CREAT,S_IRUSR|S_IWUSR);
					if (in >= 0 && out >= 0)
					{
						times_start
						for (i = 0; i < tab_size; i++)
						{
							count = read(in, ptr, tab_width);
							if (write(out, ptr, count) <= 0) break;
						}
						times_end
					}
					if (in >= 0) close(in);
					if (out >= 0) close(out);
				}
				else
				{
					FILE *fin = fopen(argv[2], "r");
					FILE *fout = fopen(argv[3], "w");
					if (fin && fout)
					{
						times_start
						for (i = 0; i<tab_size; i++)
						{
							count = fread(ptr, tab_width, 1, fin);
							if (fwrite(ptr, tab_width, 1, fout) <= 0) break;
						}
						times_end
					}
					if (fin) fclose(fin);
					if (fout) fclose(fout);
				}
				free(ptr);
			}
			else printf("Incorrect arguments!\n");
		}
	}
	else
	{
		printf("Command not recognized!\n");
	}
	printf("\n\n");

	free(start);
	free(end);
	free(rstart);
	free(rend);
	return 0;
}