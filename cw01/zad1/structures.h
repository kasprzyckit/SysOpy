#ifndef CB_STRUCTURES
#define CB_STRUCTURES

#define ARRAY_SIZE 1000
#define BLOCK_SIZE 100

typedef enum {STATIC, DYNAMIC} arrayType;

typedef struct blockArray {

	char **blocks;
	arrayType type;
	int sums[ARRAY_SIZE];
	int array_size;
	int block_size;

} blockArray;

#endif