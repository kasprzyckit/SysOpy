#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <sys/resource.h>
#define times_start getrusage(RUSAGE_SELF, start); \
					gettimeofday(rstart, NULL);
#define times_end gettimeofday(rend, NULL); \
					getrusage(RUSAGE_SELF, end); \
					print_times(start, end, rstart, rend);

#ifdef DYNAMICLOAD
	#include <dlfcn.h>
	#include "../zad1/structures.h"

	blockArray* (*createBlockArray)(arrayType, int, int);
	void (*deleteArray)(blockArray*);
	void (*addBlock)(blockArray*, char*, int);
	void (*deleteBlock)(blockArray*, int);
	int (*nearest)(blockArray*, char*, int);

#else
	#include "../zad1/char_blocks.h"
#endif

void random_string(char *s, const int len)
{
	int i;
    static const char alphanum[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    for (i = 0; i < len; ++i) {
        s[i] = alphanum[rand() % (sizeof(alphanum) - 1)];
    }
    s[len] = 0;
}

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

int main(int argc, char const *argv[])
{
	#ifdef DYNAMICLOAD
	
		void *handle = dlopen("../zad1/libCharBlocksShared.so", RTLD_LAZY);
		if(!handle){printf("NNN\n");}
		
		createBlockArray = (blockArray* (*)())dlsym(handle,"createBlockArray");
		deleteArray = (void (*)())dlsym(handle,"deleteArray");
		addBlock = (void (*)())dlsym(handle,"addBlock");
		deleteBlock = (void (*)())dlsym(handle,"deleteBlock");
		nearest = (int (*)())dlsym(handle,"nearest");

		#define createBlockArray (*createBlockArray)
		#define deleteArray (*deleteArray)
		#define addBlock (*addBlock)
		#define deleteBlock (*deleteBlock)
		#define nearest (*nearest)
	#endif

//////////////////////////////////////////////////////////

//			VALIDATING ARGUMENTS

//////////////////////////////////////////////////////////


	if (argc < 10)
	{
		printf("Too few arguments!\n");
		return 1;
	}
	int elems = atoi(argv[4]);
	int block_size = atoi(argv[5]);
	arrayType allocation;
	if (strcmp(argv[1], "STATIC") == 0) allocation = STATIC;
	else if (strcmp(argv[1], "DYNAMIC") == 0) allocation = DYNAMIC;
	else
	{
		printf("Incorrect array type!\n");
		return 1;
	}
	int tests_number = atoi(argv[2]);
	if (elems == 0 || block_size == 0 || tests_number == 0)
	{
		printf("Incorrect arguments!\n");
		return 1;
	}
	if (strcmp(argv[3], "create_table") != 0)
	{
		printf("You have to create a table as the first operation!\n");
		return 1;
	}

//////////////////////////////////////////////////////////

//		TESTING THE FUNCTIONS ACCORDING TO THE PASSED ARGUMENTS

//////////////////////////////////////////////////////////

	int seed;
    time_t tt;
    seed = time(&tt);
    srand(seed);

    int i, j, k;
    char* arg;

    char rand_string[elems][block_size+1];
    for (i = 0; i<elems; i++)
    {
    	random_string(rand_string[i], block_size);
    	rand_string[i][block_size] = 0;
    }

    blockArray* array_test = createBlockArray(allocation, elems, block_size);
    for (i = 6; i<9; i+=2)
    {
    	if (strcmp(argv[i], "search_element") == 0)
    	{
    		arg = strdup(argv[i+1]);
    		nearest(array_test, arg, block_size);
    	}
    	if (strcmp(argv[i], "remove") == 0)
    	{
    		k = atoi(argv[i+1]);
    		if (k == 0 && strcmp(argv[i+1], "0") != 0)
    		{
    			printf("Incorrect argument!\n");
    			continue;
    		}
    		for (j = 0; j < k; j++)	deleteBlock(array_test, j);
    	}
    	if (strcmp(argv[i], "add") == 0)
    	{
    		k = atoi(argv[i+1]);
    		if (k == 0 && strcmp(argv[i+1], "0") != 0)
    		{
    			printf("Incorrect argument!\n");
    			continue;
    		}
    		for (j = 0; j < k; j++)	addBlock(array_test, rand_string[j], j);
    	}
    	if (strcmp(argv[i], "remove_and_add") == 0)
    	{
    		k = atoi(argv[i+1]);
    		if (k == 0 && strcmp(argv[i+1], "0") != 0)
    		{
    			printf("Incorrect argument!\n");
    			continue;
    		}
    		for (j = 0; j < k; j++)
    		{
    			deleteBlock(array_test, j);
    			addBlock(array_test, rand_string[j], j);
    		}
    	}
    }
    deleteArray(array_test);

////////////////////////////////////////////////////////////////

//			MEASURING TIMES

////////////////////////////////////////////////////////////////

    struct rusage *start = malloc(sizeof(struct rusage));
	struct rusage *end = malloc(sizeof(struct rusage));
	struct timeval *rstart = malloc(sizeof(struct timeval));
	struct timeval *rend = malloc(sizeof(struct timeval));

	printf("\nNumber of repeats: %i\n\n", tests_number);
//////////////////////
	printf("Creating a table\n");

	blockArray* arrays[tests_number];
	times_start
	for (j=0; j<tests_number; j++) arrays[j] = createBlockArray(allocation, elems, block_size);
	times_end
	for (j=0; j<tests_number; j++) deleteArray(arrays[j]);
	printf("\n");

///////////////////////
	printf("Searching for an element\n");

	blockArray* array;
	array = createBlockArray(allocation, elems, block_size);
	char to_find[block_size];
	random_string(to_find, block_size);
	for (j = 0; j<elems; j += 2) addBlock(array, rand_string[j], j);
	times_start
	for (j=0; j<tests_number; j++) nearest(array, to_find, block_size);
	times_end
	deleteArray(array);
	printf("\n");
///////////////////////
	printf("Removing and adding blocks sequentially\n");

	array = createBlockArray(allocation, elems, block_size);
	for (j = 0; j<elems; j ++) addBlock(array, rand_string[j], j);
	times_start
	for (k=0; k<tests_number; k++)
	{
		for (j=0; j < elems; j++) deleteBlock(array, j);
		for (j=0; j < elems; j++) addBlock(array, rand_string[j], j);
	}
	times_end
	deleteArray(array);
	printf("\n");

///////////////////////
	printf("Removing and adding blocks parallelly\n");

	array = createBlockArray(allocation, elems, block_size);
	for (j = 0; j<elems; j ++) addBlock(array, rand_string[j], j);
	times_start
	for (k=0; k<tests_number; k++)
	{
		for (j=0; j < elems; j++) 
		{
			deleteBlock(array, j);
			addBlock(array, rand_string[j], j);
		}
	}
	times_end
	deleteArray(array);
///////////////////////
	printf("\n\n");

	free(start);
	free(end);
	free(rstart);
	free(rend);

	#ifdef DYNAMICLOAD
	dlclose(handle);
	#endif

	return 0;
}