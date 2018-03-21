#include "char_blocks.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

char tab[ARRAY_SIZE][BLOCK_SIZE];

int blockSum(char* block, int block_size)
{
	int i, s = 0;
	for (i = 0; i < block_size; i++) s += block[i];
	return s;
}

blockArray* createBlockArray(arrayType type, int array_size,int block_size)
{
	if (array_size > ARRAY_SIZE || block_size > BLOCK_SIZE) return NULL;
	blockArray* array = (blockArray*) calloc(1, sizeof(blockArray));
	array->type = type;
	array->array_size = array_size;
	array->block_size = block_size;
	int i;
	for (i = 0; i < array_size; i++) array->sums[i] = -1;

	if (type == STATIC)
	{
		array->blocks = NULL;
		return array;
	}
	else 
	{
		array->blocks = calloc(array_size, sizeof(char*));
	}

	return array;
}
void deleteArray(blockArray* array)
{
	if (array == NULL) return;
	if (array->type == DYNAMIC)
	{
		int i;
		for (i=0; i<array->array_size; i++)
			if (array->sums[i] != -1) free(array->blocks[i]);
		free(array->blocks);
	}
	free (array);
}

void addBlock(blockArray *array, char* block, const int index)
{
	if (array == NULL) return;
	if (index >= array->array_size || index < 0) return;
	if (array->type == DYNAMIC)
	{
		if (array->blocks[index] == NULL)
			array->blocks[index] = calloc(array->block_size, sizeof(char));
		strcpy(array->blocks[index], block);
	}
	else strcpy(tab[index], block);
	array->sums[index] = blockSum(block, array->block_size);
}

void deleteBlock(blockArray* array, int index)
{
	if (array == NULL) return;
	if (index >= array->array_size || index < 0) return;
	if (array->type == DYNAMIC && array->sums[index] != -1)
	{
		free(array->blocks[index]);
		array->blocks[index] = NULL;
	}
	array->sums[index] = -1;
}

int nearest(blockArray *array, char* block, int block_size)
{
	if (array == NULL || block == NULL) return -1;
	int sum = blockSum(block, block_size);
	int min_index = -1;
	int minimum = INT_MAX;
	int i = 0;
	for (i = 0; i < array->array_size; i++)
		if (array->sums[i] != 1 && abs(array->sums[i] - sum) < minimum)
		{
			minimum = abs(array->sums[i] - sum);
			min_index = i;
		}
	return min_index;
}