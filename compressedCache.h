/*
 * compressedCache.h
 * 
 * Created by Penggao Li
 * Last modified: 04/27/2024
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
 *                               Global variables
 *  
 * =====================================================================================
 */

#define CACHE_SIZE_KB 32
#define LINE_SIZE 32
#define SET_ASSOCIATIVITY 2
#define NUMBER_OF_SETS (CACHE_SIZE_KB * 1024 / (LINE_SIZE * SET_ASSOCIATIVITY))

extern int diff;
extern double closest;

extern long instructionCount;
extern long loadCount;
extern long loadHitCount;
extern long storeCount;
extern long storeHitCount;

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
} CompressedCacheLine;

typedef struct {
    CompressedCacheLine *lines;    // Dynamically allocated array of compressed lines
    unsigned int numberOfLines;    // Number of compressed lines in this set
    unsigned int remainingSize;    // Remaining size in bytes in this set (64 bytes/set)
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
    LRU
}ReplacementPolicy;

typedef struct {
    addr_32_bit tag;
    unsigned int index;
    unsigned int offset;
} AddressParts;

extern ReplacementPolicy RP;

/* =====================================================================================
 * 
 *                           Cache structure functions
 *  
 * =====================================================================================
 */

AddressParts extractAddressParts(addr_32_bit address);

void initializeCacheSet(CacheSet *set);

void removeLineFromCacheSet(CacheSet *set, addr_32_bit tag);

void removeLineFromCacheSetBySize(CacheSet *set, unsigned int size);

void removeLineFromCacheSetByTime(CacheSet *set, unsigned long timestamp);

bool randomEvict(CacheSet *set, CompressedCacheLine *line);

void dfs(unsigned int* nums, int size, int goal, int start, int sum, double evictedIndex);

void minDifference(unsigned int* nums, int size, int goal);

void doubleToIntegerArray(double value, int **array, int *size);

bool bestfitEvict(CacheSet *set, CompressedCacheLine *line);

void bubbleSort(unsigned long arr[], int n);

bool LRUEvict(CacheSet *set, CompressedCacheLine *line);

int addLineToCacheSet(CacheSet *set, CompressedCacheLine *line);

bool addLineToCacheSetWithRP(CacheSet *set, CompressedCacheLine *line);

void initializeCache(Cache *cache);

void freeCacheSet(CacheSet *set);

void freeCache(Cache *cache);

void initializeCacheLine(CompressedCacheLine *line, addr_32_bit tag, CompressionResult compResult);

void printCacheLineInfo(CompressedCacheLine *line);

bool ifHit(Cache *cache, addr_32_bit addr);

void cachingByAddrAndMemContent(Cache *cache, const char *filename, addr_32_bit addr);

int generateRandom(int range);

void cachingByAddrAndRandomMemContent(Cache *cache, CompressionResult *compResultArr, addr_32_bit addr, char operation);

void printSimResult(const char *filename);

void processTraceFile(Cache *cache, const char *filename, CompressionResult *compResult);

ReplacementPolicy chooseReplacementPolicy();

// void splitTraceFile(const char *inputFilename);

// void processAllFiles(Cache *cache, int fileCount, CompressionResult *compResult);

// void deleteFiles(int fileCount);