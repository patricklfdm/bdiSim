/*
 * compressedCache.h
 * 
 * Created by Penggao Li
 * Last modified: 04/29/2024
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <limits.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <string.h>
#include <unistd.h>

#include "bdi.h"

/* =====================================================================================
 * 
 *                               Macros values
 *  
 * =====================================================================================
 */

#define CACHE_SIZE_KB 32
#define LINE_SIZE 32
#define SET_ASSOCIATIVITY 2
#define NUMBER_OF_SETS (CACHE_SIZE_KB * 1024 / (LINE_SIZE * SET_ASSOCIATIVITY))

#define rrvp_max 8


/* =====================================================================================
 * 
 *                              Cache Structures
 *  
 * =====================================================================================
 */

typedef uint32_t addr_32_bit;

typedef struct {
    addr_32_bit tag;               // The tag for the compressed line
    unsigned int valid : 1;        // Valid bit
    unsigned int dirty : 1;        // Dirty bit
    unsigned int roundedCompSize;  // Rounded size to multiples of 4 bytes for storage
    CompressionResult compResult;
    unsigned long timestamp;
    unsigned int rrvp;            // Value used for RRIP 
} CompressedCacheLine;

typedef struct {
    CompressedCacheLine *lines;    // Dynamically allocated array of compressed lines
    unsigned int numberOfLines;    // Number of compressed lines in this set
    unsigned int remainingSize;    // Remaining size in bytes in this set (64 bytes/set)
    unsigned int CAMP_weight_table[8]; //one slot for every compression ratio (4byte = 0, 8byte = 1, etc)
    unsigned int CAMP_history_buffer[16];
    unsigned int CAMP_hb_count;
} CacheSet;

typedef struct {
    CacheSet sets[NUMBER_OF_SETS];
} Cache;


/* =====================================================================================
 * 
 *                           Other Struct / Enum
 *  
 * =====================================================================================
 */

typedef enum {
    RANDOM,
    BESTFIT,
    LRU,
    CAMP
}ReplacementPolicy;

typedef struct {
    addr_32_bit tag;
    unsigned int index;
    unsigned int offset;
} AddressParts;

typedef struct {
    unsigned long address;
    int ifHit;
    int ifEvict;
    unsigned int roundedCompSize;
    unsigned long timestamp;
    CompressionResult compResult;
}OutputInfo;


/* =====================================================================================
 * 
 *                           Global variables
 *  
 * =====================================================================================
 */

extern ReplacementPolicy RP;

extern int diff;
extern double closest;

extern long instructionCount;
extern long loadCount;
extern long loadHitCount;
extern long storeCount;
extern long storeHitCount;

/* =====================================================================================
 * 
 *                           Cache init/free functions
 *  
 * =====================================================================================
 */

void initializeCacheLine(CompressedCacheLine *line, addr_32_bit tag, CompressionResult compResult);

void initializeCacheSet(CacheSet *set);

void freeCacheSet(CacheSet *set);

void initializeCache(Cache *cache);

void freeCache(Cache *cache);


/* =====================================================================================
 * 
 *                           Cache accessing functions
 *  
 * =====================================================================================
 */

int addLineToCacheSet(CacheSet *set, CompressedCacheLine *line);

bool addLineToCacheSetWithRP(CacheSet *set, CompressedCacheLine *line, OutputInfo *info, FILE *csv);

void removeLineFromCacheSet(CacheSet *set, addr_32_bit tag);

void removeLineFromCacheSetBySize(CacheSet *set, unsigned int size, OutputInfo *evictInfo);

void removeLineFromCacheSetByTime(CacheSet *set, unsigned long timestamp, OutputInfo *evictInfo);

bool ifHit(Cache *cache, addr_32_bit addr, OutputInfo *info);

void cachingByAddrAndRandomMemContent(Cache *cache, CompressionResult *compResultArr, addr_32_bit addr, char operation, FILE *csv);


/* =====================================================================================
 * 
 *                           Cache util functions
 *  
 * =====================================================================================
 */

AddressParts extractAddressParts(addr_32_bit address);

void dfs(unsigned int* nums, int size, int goal, int start, int sum, double evictedIndex);

void minDifference(unsigned int* nums, int size, int goal);

void doubleToIntegerArray(double value, int **array, int *size);

void bubbleSort(unsigned long arr[], int n);

int generateRandom(int range);

void printCacheLineInfo(CompressedCacheLine *line);

void printSimResult(const char *filename);


/* =====================================================================================
 * 
 *                           Cache replacement functions
 *  
 * =====================================================================================
 */

ReplacementPolicy chooseReplacementPolicy();

bool randomEvict(CacheSet *set, CompressedCacheLine *line, OutputInfo *info, FILE *csv);

bool bestfitEvict(CacheSet *set, CompressedCacheLine *line, OutputInfo *info, FILE *csv);

bool LRUEvict(CacheSet *set, CompressedCacheLine *line, OutputInfo *info, FILE *csv);


/* =====================================================================================
 * 
 *                           Output file processing functions
 *  
 * =====================================================================================
 */

char *generateOutputInfo(OutputInfo info);

void processTraceFile(Cache *cache, const char *filename, CompressionResult *compResult);

char *processTraceFileName(const char *filename);
