/*
 * compressedCache.c
 * 
 * Created by Penggao Li
 * Last modified: 04/27/2024
 */

#include "compressedCache.h"

/* =====================================================================================
 * 
 *                           Cache structure function
 *  
 * =====================================================================================
 */

// Extract tag, index, and offset from 32-bit address
AddressParts extractAddressParts(addr_32_bit address) {
    AddressParts parts;
    parts.offset = address & 0x1F;
    parts.index = (address >> 5) & 0x1FF;
    parts.tag = address >> 14;
    return parts;
}

void initializeCacheSet(CacheSet *set) {
    // printf("\nInitializing cacheset...\n");
    set->lines = NULL;  // Initially, no lines are allocated
    set->numberOfLines = 0;
    set->remainingSize = 64;  // Start with the full set size available
    // printf("\nInitialize cacheset complete\n");
}

void removeLineFromCacheSet(CacheSet *set, addr_32_bit tag) {
    // printf("\nRemoving a line from cacheset...\n");
    for(int i = 0; i < set->numberOfLines; i++){
        if(set->lines[i].tag == tag){
            set->remainingSize += set->lines[i].roundedCompSize; // Reclaim the space
            // Move the last line to the removed spot to keep array compact
            if(i != set->numberOfLines - 1){
                set->lines[i] = set->lines[set->numberOfLines - 1];
            }
            set->numberOfLines--;
            // printf("\nRemoved a line from cacheset\n");
            return;
        }
    }
    // printf("\nTry to removed a line BUT NOT FOUND!!!\n");
}

void removeLineFromCacheSetBySize(CacheSet *set, unsigned int size) {
    // printf("\nRemoving a line from cacheset...\n");
    for(int i = 0; i < set->numberOfLines; i++){
        if(set->lines[i].roundedCompSize == size){
            set->remainingSize += set->lines[i].roundedCompSize; // Reclaim the space
            // Move the last line to the removed spot to keep array compact
            if(i != set->numberOfLines - 1){
                set->lines[i] = set->lines[set->numberOfLines - 1];
            }
            set->numberOfLines--;

            // printf("\nRemoved cacheline size: %d\n", size);
            return;
        }
    }
    // printf("\nTry to removed a line by size BUT NOT FOUND!!!\n");
}

void removeLineFromCacheSetByTime(CacheSet *set, unsigned long timestamp) {
    // printf("\nRemoving a line from cacheset...\n");
    unsigned int size = 0;
    for(int i = 0; i < set->numberOfLines; i++){
        if(set->lines[i].timestamp == timestamp){
            size = set->lines[i].roundedCompSize;
            set->remainingSize += size; // Reclaim the space
            // Move the last line to the removed spot to keep array compact
            if(i != set->numberOfLines - 1){
                set->lines[i] = set->lines[set->numberOfLines - 1];
            }
            set->numberOfLines--;

            // printf("\nRemoved cacheline size: %d, timestamp: %ld\n", size, timestamp);
            return;
        }
    }
    // printf("\nTry to removed a line by size BUT NOT FOUND!!!\n");
}

bool randomEvict(CacheSet *set, CompressedCacheLine *line){

    if (set->numberOfLines == 0) {
        perror("ERROR calling random!!!");
        return false;
    }

    int evictIndex = 0;
    srand(time(NULL));

    while (set->remainingSize < line->roundedCompSize)
    {
        evictIndex = rand() % set->numberOfLines;
        // printf("\nRANDOM: evict %d\n", evictIndex);
        removeLineFromCacheSet(set, set->lines[evictIndex].tag);
    }

    return true;
}

void dfs(unsigned int* nums, int size, int goal, int start, int sum, double evictedIndex) {

    // printf("\nDFS called. Sum: %d\n", sum);

    if (sum - goal < diff && goal <= sum) {
        diff = sum - goal;
        closest = evictedIndex;
    }
    for (int i = start; i < size; ++i) {
        dfs(nums, size, goal, i + 1, sum + nums[i], evictedIndex * 100 + nums[i]);
    }
}

void minDifference(unsigned int* nums, int size, int goal) {

    // printf("\n[Start DFS for goal: %d]\n", goal);

    if (size == 0) {
        return;
    }

    dfs(nums, size, goal, 0, 0, 0);
}

void doubleToIntegerArray(double value, int **array, int *size) {
    // Temporary buffer to hold the integer part of the double as a string
    char buffer[30];
    
    // Convert double to string, ignoring the decimal part
    sprintf(buffer, "%.0f", value);
    
    // Determine the size of the array
    int len = strlen(buffer);
    *size = (len + 1) / 2;  // Calculate number of integer pairs, rounding up if odd
    *array = (int *)malloc(*size * sizeof(int));
    if (*array == NULL) {
        perror("Memory allocation failed");
        exit(EXIT_FAILURE);
    }

    // Initialize the array index and start from the end of the string
    int index = 0;
    for (int i = len - 1; i >= 0; i -= 2) {
        // Check if it's the last single digit
        if (i == 0) {
            (*array)[index++] = buffer[i] - '0';  // Convert char to int
        } else {
            // Process two digits at a time
            (*array)[index++] = (buffer[i - 1] - '0') * 10 + (buffer[i] - '0');
        }
    }

    for (int i = 0; i < *size / 2; i++) {
        int temp = (*array)[i];
        (*array)[i] = (*array)[*size - i - 1];
        (*array)[*size - i - 1] = temp;
    }
}

bool bestfitEvict(CacheSet *set, CompressedCacheLine *line){

    if (set->numberOfLines == 0) {
        perror("ERROR calling random!!!");
        return false;
    }

    unsigned int sizes[set->numberOfLines];
    unsigned int goalSize = line->roundedCompSize - set->remainingSize;

    for(int i = 0; i < set->numberOfLines; i++){
        sizes[i] = set->lines[i].roundedCompSize;
    }

    minDifference(sizes, set->numberOfLines, goalSize);

    int *intArray = NULL;
    int arrSize = 0;
    doubleToIntegerArray(closest, &intArray, &arrSize);

    for(int i = 0; i < arrSize; i++){
        removeLineFromCacheSetBySize(set, intArray[i]);
    }

    diff = INT_MAX;
    closest = 0;
    free(intArray);
    intArray = NULL;

    // printf("\nBESTFIT remove total: %d lines\n", arrSize);

    return true;
}

// Descending
void bubbleSort(unsigned long arr[], int n) {
    int i, j;
    unsigned long temp;
    for (i = 0; i < n - 1; i++) {
        // Last i elements are already in place
        for (j = 0; j < n - i - 1; j++) {
            if (arr[j] < arr[j + 1]) {
                temp = arr[j];
                arr[j] = arr[j + 1];
                arr[j + 1] = temp;
            }
        }
    }
}


bool LRUEvict(CacheSet *set, CompressedCacheLine *line){

    if (set->numberOfLines == 0) {
        perror("ERROR calling random!!!");
        return false;
    }

    int count = set->numberOfLines;

    unsigned long timeArr[count];

    for(int i = 0; i < count; i++){
        timeArr[i] = set->lines[i].timestamp;
        // printf("\ntime: %ld\n", timeArr[i]);
    }

    bubbleSort(timeArr, count);
    int index = 0;

    while (set->remainingSize < line->roundedCompSize)
    {
        removeLineFromCacheSetByTime(set, timeArr[index]);
        index++;
        if(index >= count){
            perror("ERROR in LRU!!!");
            return false;
        }
    }

    return true;
}

int addLineToCacheSet(CacheSet *set, CompressedCacheLine *line) {
    // printf("\nAdding new line to cacheset...\n");
    if (set->remainingSize >= line->roundedCompSize) {
        // Ensure enough space
        set->lines = realloc(set->lines, (set->numberOfLines + 1) * sizeof(CompressedCacheLine));
        if (set->lines == NULL) {
            return -1; // Memory allocation failed
        }
        set->lines[set->numberOfLines] = *line; // Copy the line into the set
        set->numberOfLines++;
        set->remainingSize -= line->roundedCompSize; // Decrease remaining size
        // printf("\nNew line added.\n");
        return 0; // Success
    }
    return -1; // Not enough space
}

bool addLineToCacheSetWithRP(CacheSet *set, CompressedCacheLine *line){

    if(addLineToCacheSet(set, line) == -1){
        switch(RP){
            case RANDOM:
            randomEvict(set, line);
            break;
            case BESTFIT:
            bestfitEvict(set, line);
            break;
            case LRU:
            LRUEvict(set, line);
            break;
            default:
            randomEvict(set, line);
            break;
        }
        if(addLineToCacheSet(set, line) == 0){
            return true;
        }else{
            perror("ERROR adding new line!!!");
            return false;
        }
    }

    return true;
}

void initializeCache(Cache *cache) {
    // printf("\nInitializing cache...\n");
    for (int i = 0; i < NUMBER_OF_SETS; i++) {
        initializeCacheSet(&(cache->sets[i]));
    }
    // printf("\nInitialize cache complete\n");
}

void freeCacheSet(CacheSet *set) {
    // printf("\nFreeing cacheset...\n");
    if (set->lines != NULL) {
        free(set->lines);  // Free the lines
        set->lines = NULL;
    }
    set->numberOfLines = 0;
    set->remainingSize = LINE_SIZE * SET_ASSOCIATIVITY;  // Reset the remaining size
    // printf("\nFreed cacheset\n");
}

void freeCache(Cache *cache) {
    // printf("\nFreeing cache...\n");
    for (int i = 0; i < NUMBER_OF_SETS; i++) {
        freeCacheSet(&(cache->sets[i]));
    }
    // printf("\nFreed cache\n");
}

void initializeCacheLine(CompressedCacheLine *line, addr_32_bit tag, CompressionResult compResult) {
    // printf("\nInitializing cacheline...\n");
    line->tag = tag;
    line->valid = 1;
    line->dirty = 0;
    line->compResult = compResult;
    line->roundedCompSize = (compResult.compSize + 3) & ~3;
    line->timestamp = 0;
    // printf("\nInitialized cacheline\n");
}

void printCacheLineInfo(CompressedCacheLine *line) {
    printf("\n================================================");
    printf("\nCache Line Information:\n");
    if(line == NULL){
        printf("\nEmpty line!!!\n");
        return;
    }
    printf("Tag: 0x%X\n", line->tag);
    printf("Valid: %s\n", line->valid ? "Yes" : "No");
    printf("Dirty: %s\n", line->dirty ? "Yes" : "No");
    printf("Rounded Compressed Size: %u bytes\n", line->roundedCompSize);
    printf("Compression Results:\n");
    printf("  - isZero: %u\n", line->compResult.isZero);
    printf("  - isSame: %u\n", line->compResult.isSame);
    printf("  - Comp Size: %u\n", line->compResult.compSize);
    printf("  - K: %u\n", line->compResult.K);
    printf("  - BaseNum: %u\n", line->compResult.BaseNum);
    printf("Timestamp: %ld\n", line->timestamp);
    printf("================================================\n");
}

bool ifHit(Cache *cache, addr_32_bit addr){

    AddressParts parts = extractAddressParts(addr);
    // printf("Address: 0x%X\nTag: 0x%X\nIndex: %u\nOffset: %u\n",
        //    addr, parts.tag, parts.index, parts.offset);

    CacheSet set = cache->sets[parts.index];

    bool flag = false;

    if(set.lines == NULL || set.numberOfLines == 0){
        return false;
    }

    for(int i = 0; i < set.numberOfLines; i++){
        if(set.lines[i].tag == parts.tag){
            flag = true;
            set.lines[i].timestamp = 0;
        }else{
            set.lines[i].timestamp++;
        }
    }
    return flag;
}

void cachingByAddrAndMemContent(Cache *cache, const char *filename, addr_32_bit addr){

    if(ifHit(cache, addr)){
        // printf("\n[HIT]!!!!!\n");
        return;
    }

    AddressParts parts = extractAddressParts(addr);
    // printf("Address: 0x%X\nTag: 0x%X\nIndex: %u\nOffset: %u\n",
        //    addr, parts.tag, parts.index, parts.offset);

    CompressionResult compResult;
    generateCompressedData(filename, &compResult);
    
    CompressedCacheLine *newLine = (CompressedCacheLine *)malloc(sizeof(CompressedCacheLine));
    initializeCacheLine(newLine, parts.tag, compResult);

    // AddressParts addressParts = extractAddressParts(newLine->tag);

    addLineToCacheSetWithRP(&((*cache).sets[parts.index]), newLine);

    // printCacheLineInfo((*cache).sets[parts.index].lines);
    // printf("\n-- [Cacheset left: %d, num: %d] --\n\n", (*cache).sets[parts.index].remainingSize, (*cache).sets[parts.index].numberOfLines);
}

int generateRandom(int range){
    srand(time(NULL));
    return rand() % range;
}

void cachingByAddrAndRandomMemContent(Cache *cache, CompressionResult *compResultArr, addr_32_bit addr, char operation){

    if(ifHit(cache, addr)){

        if(operation == 'l'){
            loadHitCount++;
        }else if(operation == 's'){
            storeHitCount++;
        }
        // printf("\n[HIT]!!!!!\n");
        return;
    }

    AddressParts parts = extractAddressParts(addr);
    // printf("Address: 0x%X\nTag: 0x%X\nIndex: %u\nOffset: %u\n",
    //        addr, parts.tag, parts.index, parts.offset);

    int randomNum = generateRandom(5);
    CompressionResult compResult = compResultArr[randomNum];
    
    CompressedCacheLine *newLine = (CompressedCacheLine *)malloc(sizeof(CompressedCacheLine));
    initializeCacheLine(newLine, parts.tag, compResult);

    addLineToCacheSetWithRP(&((*cache).sets[parts.index]), newLine);

    // printCacheLineInfo((*cache).sets[parts.index].lines);
    // printf("\n-- [Cacheset left: %d, num: %d] --\n\n", (*cache).sets[parts.index].remainingSize, (*cache).sets[parts.index].numberOfLines);
}

void printSimResult(const char *filename){
    double loadHitRate = ((double)loadHitCount)/((double)loadCount);
    double storeHitRate = ((double)storeHitCount)/((double)storeCount);
    double totalHitRate = ((double)(loadHitCount + storeHitCount))/((double)instructionCount);
    printf("\n\n==========================================================\n");
    printf("File: %s\n", filename);
    printf("Instructions: %ld\n", instructionCount);
    printf("        Load: %ld\n", loadCount);
    printf("       Store: %ld\n", storeCount);
    printf("----------------------------------------------------------\n");
    printf(" loadHitRate: %f\n", loadHitRate);
    printf("StoreHitRate: %f\n", storeHitRate);
    printf("TotalHitRate: %f\n", totalHitRate);
    printf("==========================================================\n");
}

void processTraceFile(Cache *cache, const char *filename, CompressionResult *compResult) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        perror("Failed to open file");
        return;
    }

    char line[1024];
    char operation;  // 'l' for load, 's' for store
    unsigned long address;

    while (fgets(line, sizeof(line), file) != NULL) {
        instructionCount++;
        // if(instructionCount % 10000 == 0){
        //     int n = instructionCount / 10000;
        //     printf("\nProcessed %d x 10k...\n", n);
        // }
        if (sscanf(line, "%c 0x%lx", &operation, &address) == 2) {
            if(operation == 'l'){
                loadCount++;
            }else if(operation == 's'){
                storeCount++;
            }
            cachingByAddrAndRandomMemContent(cache, compResult, address, operation);
        } else {
            fprintf(stderr, "Error parsing line: %s", line);
        }
    }

    fclose(file);
}

ReplacementPolicy chooseReplacementPolicy() {
    int choice;
    printf("\nSelect a replacement policy:\n");
    printf("1. RANDOM\n");
    printf("2. BESTFIT\n");
    printf("3. LRU\n");
    printf("Enter your choice (1-3): ");
    scanf("%d", &choice);

    switch (choice) {
        case 1:
        printf("Using replacement policy: RANDOM\n");
        return RANDOM;
        case 2:
        printf("Using replacement policy: BESTFIT\n");
        return BESTFIT;
        case 3:
        printf("Using replacement policy: LRU\n");
        return LRU;
        default:
            printf("Invalid choice, defaulting to LRU.\n");
            return LRU;
    }
}

// void splitTraceFile(const char *inputFilename) {
//     FILE *inputFile = fopen(inputFilename, "r");
//     if (inputFile == NULL) {
//         perror("Failed to open input file");
//         exit(EXIT_FAILURE);
//     }

//     int fileCount = 1;
//     char outputFilename[256];
//     FILE *outputFile;
//     char line[1024];
//     int lineCount = 0;

//     sprintf(outputFilename, "testTraces/tempTests/gcc_%d.trace", fileCount);
//     outputFile = fopen(outputFilename, "w");
//     if (outputFile == NULL) {
//         perror("Failed to open output file");
//         fclose(inputFile);
//         exit(EXIT_FAILURE);
//     }

//     while (fgets(line, sizeof(line), inputFile) != NULL) {
//         if (lineCount == 100000) {
//             fclose(outputFile);
//             fileCount++;
//             sprintf(outputFilename, "testTraces/tempTests/gcc_%d.trace", fileCount);
//             outputFile = fopen(outputFilename, "w");
//             if (outputFile == NULL) {
//                 perror("Failed to open next output file");
//                 fclose(inputFile);
//                 exit(EXIT_FAILURE);
//             }
//             lineCount = 0;
//         }
//         fprintf(outputFile, "%s", line);
//         lineCount++;
//     }

//     fclose(outputFile);
//     fclose(inputFile);
// }

// void processAllFiles(Cache *cache, int fileCount, CompressionResult *compResult) {
//     char filename[256];
//     for (int i = 1; i <= fileCount; i++) {
//         sprintf(filename, "testTraces/tempTests/gcc_%d.trace", i);
//         processTraceFile(cache, filename, compResult);
//     }
// }

// void deleteFiles(int fileCount) {
//     char filename[256];
//     for (int i = 1; i <= fileCount; i++) {
//         sprintf(filename, "testTraces/tempTests/gcc_%d.trace", i);
//         if (remove(filename) != 0) {
//             perror("Error deleting file");
//         } else {
//             printf("Deleted file: %s\n", filename);
//         }
//     }
// }