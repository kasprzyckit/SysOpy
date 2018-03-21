#ifndef CHAR_BLOCKS
#define CHAR_BLOCKS

#include "structures.h"

blockArray* createBlockArray(arrayType type, int array_size,int block_size);
void deleteArray(blockArray* array);
void addBlock(blockArray *array, char* block, int index);
void deleteBlock(blockArray* array, int index);
int nearest(blockArray *array, char* block, int block_size);

#endif