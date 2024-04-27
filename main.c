/*
 * main.c
 * 
 * Created by Penggao Li
 * Last modified: 04/27/2024
 */

#include "compressedCache.h"

/* =====================================================================================
 * 
 *                               Global variables
 *  
 * =====================================================================================
 */

int diff = INT_MAX;
double closest = 0;

long instructionCount = 0;
long loadCount = 0;
long loadHitCount = 0;
long storeCount = 0;
long storeHitCount = 0;
ReplacementPolicy RP = LRU;

/* =====================================================================================
 * 
 *                               main function
 *  
 * =====================================================================================
 */

int main() {

    char traceName[64];
    // const char *defaultTestTrace = "testTraces/test.trace";

    printf("Enter the trace file name: ");
    if (fgets(traceName, sizeof(traceName), stdin) == NULL) {
        printf("Error reading input.\n");
        return 1;
    }

    traceName[strcspn(traceName, "\n")] = 0;  // Remove newline character

    RP = chooseReplacementPolicy();

    const char *filename1 = "testHex/hex1.txt";
    const char *filename2 = "testHex/hex2.txt";
    const char *filename3 = "testHex/hex3.txt";
    const char *filename4 = "testHex/hex4.txt";
    const char *filename5 = "testHex/hex5.txt";
    
    CompressionResult compResult[5];
    generateCompressedData(filename1, &compResult[0]);
    generateCompressedData(filename2, &compResult[1]);
    generateCompressedData(filename3, &compResult[2]);
    generateCompressedData(filename4, &compResult[3]);
    generateCompressedData(filename5, &compResult[4]);

    Cache myCache;
    initializeCache(&myCache);

    clock_t start, end;
    double cpu_time_used;

    start = clock();
    
    processTraceFile(&myCache, traceName, compResult);
    // processTraceFile(&myCache, defaultTestTrace, compResult);

    printSimResult(traceName);

    end = clock();
    cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;

    printf("Execution time: %f seconds\n", cpu_time_used);

    freeCache(&myCache);
    printf("Cache has been successfully freed.\n");
    return 0;
}