/*
 * compressedCache.c
 * 
 * Created by Penggao Li
 * Last modified: 04/29/2024
 */

#include "compressedCache.h"


/* =====================================================================================
 * 
 *                           Cache init/free functions
 *  
 * =====================================================================================
 */

void initializeCacheLine(CompressedCacheLine *line, addr_32_bit tag, CompressionResult compResult) {
    // printf("\nInitializing cacheline...\n");
    line->tag = tag;
    line->valid = 1;
    line->dirty = 0;
    line->compResult = compResult;
    line->roundedCompSize = (compResult.compSize + 3) & ~3;
    line->timestamp = 0;
    line->rrvp = rrvp_max;
    // printf("\nInitialized cacheline\n");
}

void initializeCacheSet(CacheSet *set) {
    // printf("\nInitializing cacheset...\n");
    set->lines = NULL;
    set->numberOfLines = 0;
    set->remainingSize = 64;
    set->CAMP_hb_count = 0;
    for(int i = 0; i < 8; i++){
        set->CAMP_weight_table[i] = i+1;
    }
    for(int i = 0; i < 16; i++){
        set->CAMP_history_buffer[i] = 0;
    }
    // printf("\nInitialize cacheset complete\n");
}

void freeCacheSet(CacheSet *set) {
    // printf("\nFreeing cacheset...\n");
    if (set->lines != NULL) {
        free(set->lines);  // Free the lines
        set->lines = NULL;
    }
    set->numberOfLines = 0;
    set->remainingSize = LINE_SIZE * SET_ASSOCIATIVITY;
    set->CAMP_hb_count = 0;
    // printf("\nFreed cacheset\n");
}

void initializeCache(Cache *cache) {
    //printf("\nInitializing cache...\n");
    for (int i = 0; i < NUMBER_OF_SETS; i++) {
        initializeCacheSet(&(cache->sets[i]));
    }
    cache->CAMP_training_counter = 160;
    //printf("\nInitialize cache complete\n");
}

void freeCache(Cache *cache) {
    // printf("\nFreeing cache...\n");
    for (int i = 0; i < NUMBER_OF_SETS; i++) {
        freeCacheSet(&(cache->sets[i]));
    }
    // printf("\nFreed cache\n");
}


/* =====================================================================================
 * 
 *                           Cache accessing functions
 *  
 * =====================================================================================
 */

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

bool addLineToCacheSetWithRP(CacheSet *set, CompressedCacheLine *line, OutputInfo *info, FILE *csv){

    if(addLineToCacheSet(set, line) == -1){

        info->ifEvict = 1;

        switch(RP){
            case RANDOM:
            randomEvict(set, line, info, csv);
            break;
            case BESTFIT:
            bestfitEvict(set, line, info, csv);
            break;
            case LRU:
            LRUEvict(set, line, info, csv);
            break;
            case CAMP:
            CAMPEvict(set, line, info, csv);
            break;
            default:
            randomEvict(set, line, info, csv);
            break;
        }
        if(addLineToCacheSet(set, line) == 0){

            info->ifEvict = 0;

            return true;
        }else{
            perror("ERROR adding new line!!!");
            return false;
        }
    }

    info->ifEvict = 0;

    return true;
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

void removeLineFromCacheSetBySize(CacheSet *set, unsigned int size, OutputInfo *evictInfo) {
    // printf("\nRemoving a line from cacheset...\n");
    for(int i = 0; i < set->numberOfLines; i++){
        if(set->lines[i].roundedCompSize == size){

            evictInfo->compResult = set->lines[i].compResult;
            evictInfo->roundedCompSize = set->lines[i].roundedCompSize;
            evictInfo->timestamp = set->lines[i].timestamp;

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

void removeLineFromCacheSetByTime(CacheSet *set, unsigned long timestamp, OutputInfo *evictInfo) {
    // printf("\nRemoving a line from cacheset...\n");
    unsigned int size = 0;
    for(int i = 0; i < set->numberOfLines; i++){
        if(set->lines[i].timestamp == timestamp){

            evictInfo->compResult = set->lines[i].compResult;
            evictInfo->roundedCompSize = set->lines[i].roundedCompSize;
            evictInfo->timestamp = set->lines[i].timestamp;

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

bool ifHit(Cache *cache, addr_32_bit addr, OutputInfo *info){

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

            info->compResult = set.lines[i].compResult;
            info->ifEvict = 0;
            info->ifHit = 1;
            info->roundedCompSize = set.lines[i].roundedCompSize;
            info->timestamp = set.lines[i].timestamp;

            flag = true;
            set.lines[i].timestamp = 0;
            updateCamp(&(cache->sets[parts.index]), set.lines[i].roundedCompSize);
            if(set.lines[i].rrvp != 0) set.lines[i].rrvp -= 1;
        }else{
            set.lines[i].timestamp++;
        }
    }
    return flag;
}

void cachingByAddrAndRandomMemContent(Cache *cache, CompressionResult *compResultArr, addr_32_bit addr, char operation, FILE *csv){

    OutputInfo info;
    info.address = addr;
    char *outputInfo = NULL;

    if(ifHit(cache, addr, &info)){

        if(operation == 'l'){
            loadHitCount++;
        }else if(operation == 's'){
            storeHitCount++;
        }

        outputInfo = generateOutputInfo(info);
        fprintf(csv, "%s", outputInfo);
        free(outputInfo);
        outputInfo = NULL;

        // printf("\n[HIT]!!!!!\n");
        return;
    }

    info.ifHit = 0;

    AddressParts parts = extractAddressParts(addr);
    // printf("Address: 0x%X\nTag: 0x%X\nIndex: %u\nOffset: %u\n",
    //        addr, parts.tag, parts.index, parts.offset);

    int randomNum = generateRandom(5);
    CompressionResult compResult = compResultArr[randomNum];

    info.compResult = compResult;
    
    CompressedCacheLine *newLine = (CompressedCacheLine *)malloc(sizeof(CompressedCacheLine));
    initializeCacheLine(newLine, parts.tag, compResult);

    info.roundedCompSize = newLine->roundedCompSize;
    info.timestamp = 0;
    
    addLineToCacheSetWithRP(&((*cache).sets[parts.index]), newLine, &info, csv);

    outputInfo = generateOutputInfo(info);
    fprintf(csv, "%s", outputInfo);
    free(outputInfo);
    outputInfo = NULL;

    // printCacheLineInfo((*cache).sets[parts.index].lines);
    // printf("\n-- [Cacheset left: %d, num: %d] --\n\n", (*cache).sets[parts.index].remainingSize, (*cache).sets[parts.index].numberOfLines);
}

void updateCamp(CacheSet *set, int size){
    if(set->CAMP_hb_count == 16) {
        printf("history full\n");
        return;
    }
    int setCount = set->CAMP_hb_count;
    set->CAMP_history_buffer[setCount] = size;
    set->CAMP_hb_count = setCount + 1;
    if(set->CAMP_hb_count == 16){
        set->CAMP_hb_count = 0;
    }
    return;
}

/* =====================================================================================
 * 
 *                           Cache util functions
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

int generateRandom(int range){
    srand(time(0) + rand());
    return rand() % range;
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
    printf("RRVP: %d\n", line->rrvp);
    printf("================================================\n");
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

/* =====================================================================================
 * 
 *                           Cache replacement functions
 *  
 * =====================================================================================
 */

ReplacementPolicy chooseReplacementPolicy() {
    int choice;
    printf("\nSelect a replacement policy:\n");
    printf("1. RANDOM\n");
    printf("2. BESTFIT\n");
    printf("3. LRU\n");
    printf("4. CAMP\n");
    printf("Enter your choice (1-4): ");
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
        case 4:
        printf("Using replacement policy: CAMP\n");
        return CAMP;
        default:
            printf("Invalid choice, defaulting to LRU.\n");
            return LRU;
    }
}

bool randomEvict(CacheSet *set, CompressedCacheLine *line, OutputInfo *info, FILE *csv){

    if (set->numberOfLines == 0) {
        perror("ERROR calling random!!!");
        return false;
    }

    OutputInfo evictInfo;
    evictInfo.address = info->address;
    evictInfo.compResult = info->compResult;
    evictInfo.ifEvict = info->ifEvict;
    evictInfo.ifHit =info->ifHit;
    evictInfo.roundedCompSize = info->roundedCompSize;
    evictInfo.timestamp = info->timestamp;

    int evictIndex = 0;
    srand(time(0) + rand());

    char *outputInfo = NULL;

    while (set->remainingSize < line->roundedCompSize)
    {
        evictIndex = rand() % set->numberOfLines;
        // printf("\nRANDOM: evict %d\n", evictIndex);

        evictInfo.compResult = set->lines[evictIndex].compResult;
        evictInfo.roundedCompSize = set->lines[evictIndex].roundedCompSize;
        evictInfo.timestamp = set->lines[evictIndex].timestamp;

        outputInfo = generateOutputInfo(evictInfo);
        fprintf(csv, "%s", outputInfo);
        free(outputInfo);
        outputInfo = NULL;

        removeLineFromCacheSet(set, set->lines[evictIndex].tag);
    }

    return true;
}

bool bestfitEvict(CacheSet *set, CompressedCacheLine *line, OutputInfo *info, FILE *csv){

    if (set->numberOfLines == 0) {
        perror("ERROR calling random!!!");
        return false;
    }

    OutputInfo evictInfo;
    evictInfo.address = info->address;
    evictInfo.compResult = info->compResult;
    evictInfo.ifEvict = info->ifEvict;
    evictInfo.ifHit =info->ifHit;
    evictInfo.roundedCompSize = info->roundedCompSize;
    evictInfo.timestamp = info->timestamp;

    char *outputInfo = NULL;

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

        removeLineFromCacheSetBySize(set, intArray[i], &evictInfo);
        outputInfo = generateOutputInfo(evictInfo);
        fprintf(csv, "%s", outputInfo);
        free(outputInfo);
        outputInfo = NULL;
    }

    diff = INT_MAX;
    closest = 0;
    free(intArray);
    intArray = NULL;

    // printf("\nBESTFIT remove total: %d lines\n", arrSize);

    return true;
}

bool LRUEvict(CacheSet *set, CompressedCacheLine *line, OutputInfo *info, FILE *csv){

    if (set->numberOfLines == 0) {
        perror("ERROR calling random!!!");
        return false;
    }

    OutputInfo evictInfo;
    evictInfo.address = info->address;
    evictInfo.compResult = info->compResult;
    evictInfo.ifEvict = info->ifEvict;
    evictInfo.ifHit =info->ifHit;
    evictInfo.roundedCompSize = info->roundedCompSize;
    evictInfo.timestamp = info->timestamp;

    char *outputInfo = NULL;

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
        removeLineFromCacheSetByTime(set, timeArr[index], &evictInfo);
        index++;

        outputInfo = generateOutputInfo(evictInfo);
        fprintf(csv, "%s", outputInfo);
        free(outputInfo);
        outputInfo = NULL;

        if(index >= count){
            perror("ERROR in LRU!!!");
            return false;
        }
    }

    return true;
}

bool CAMPEvict(CacheSet *set, CompressedCacheLine *line, OutputInfo *info, FILE *csv){
    //printf("\nCAMPEvict\n");
    //printCacheLineInfo(line);
    if (set->numberOfLines == 0) {
        perror("ERROR calling random!!!");
        return false;
    }

    OutputInfo evictInfo;
    evictInfo.address = info->address;
    evictInfo.compResult = info->compResult;
    evictInfo.ifEvict = info->ifEvict;
    evictInfo.ifHit =info->ifHit;
    evictInfo.roundedCompSize = info->roundedCompSize;
    evictInfo.timestamp = info->timestamp;

    char *outputInfo = NULL;

    while (set->remainingSize < line->roundedCompSize) {
        int victim_idx = -1;
        int victim_rrvp = -1;
        int victim_mve = -1;
        int highest_rrvp = -1;
        if(set->numberOfLines == 0){
            printf("Error Cache line too large!!");
            return false;
        }
        //Find the victim with highest MVE
        //printf("\nSearching set\n");
        for(int i = 0; i < set->numberOfLines; i++){
            //printf("\nidx: %d num_lines:%d\n", i, set->numberOfLines);
            //printCacheLineInfo(&set->lines[i]);
            int candidate_rrvp = set->lines[i].rrvp;
            //printf("\nc_rrvp: %d\n",candidate_rrvp);
            int candidate_compression_idx = ((set->lines[i].roundedCompSize) / 4) -1;
            //printf("\nc_comp_idx: %d\n",candidate_compression_idx);
            int candidate_compression = set->CAMP_weight_table[candidate_compression_idx];
            //printf("\nc_comp: %d\n", candidate_compression);
            int candidate_MVE = candidate_rrvp / candidate_compression;
            //printf("\nc_rrvp: %d, c_comp: %d, c_mve: %d\n",candidate_rrvp, candidate_compression, candidate_MVE);
            if(candidate_rrvp > victim_rrvp){
                highest_rrvp = candidate_rrvp;
            }
            if(candidate_MVE > victim_mve){
                victim_idx = i;
                victim_rrvp = candidate_rrvp;
                victim_mve = candidate_MVE;
            }
        }
        if(victim_idx == -1){
            printf("Error: Couldn't find evict target");
            return false;
        }

        //Update evict info
        evictInfo.compResult = set->lines[victim_idx].compResult;
        evictInfo.roundedCompSize = set->lines[victim_idx].roundedCompSize;
        evictInfo.timestamp = set->lines[victim_idx].timestamp;

        //Reclaim the space
        int size = set->lines[victim_idx].roundedCompSize;
        set->remainingSize += size;

        // Move the last line to the removed spot to keep array compact
        if(victim_idx != set->numberOfLines - 1){
            set->lines[victim_idx] = set->lines[set->numberOfLines - 1];
        }
        set->numberOfLines--;

        //if highest_rrvp != rrvp_max, add the diff to every rrpv
        int diff = rrvp_max - highest_rrvp;
        if(diff < 0){
            printf("ERROR: rrvp over max");
            return false;
        }
        if(diff > 0){
            for(int j = 0; j < set->numberOfLines; j++){
                if(set->lines[j].rrvp + diff > rrvp_max){
                    set->lines[j].rrvp = rrvp_max;
                } else {
                    set->lines[j].rrvp += diff;
                }
            }
        }
        updateCamp(set, line->roundedCompSize);
        outputInfo = generateOutputInfo(evictInfo);
        fprintf(csv, "%s", outputInfo);
        free(outputInfo);
        outputInfo = NULL;
        //printf("\nRemoved cacheline idx: %d, size: %d, MVE: %d\n", victim_idx, size, victim_mve);
    }
    //printf("\nCAMPEvict exit\n");
    return true;
}

/* =====================================================================================
 * 
 *                           Output file processing functions
 *  
 * =====================================================================================
 */

char *generateOutputInfo(OutputInfo info) {
    int size = 150;
    char *output = malloc(size * sizeof(char));
    if (output == NULL) {
        perror("Failed to allocate memory");
        exit(EXIT_FAILURE);
    }

    // Format the output string
    snprintf(output, size, "%lx,%d,%d,%u,%lu,%u,%u,%u,%u,%u\n",
             info.address,
             info.ifHit,
             info.ifEvict,
             info.roundedCompSize,
             info.timestamp,
             info.compResult.isZero,
             info.compResult.isSame,
             info.compResult.compSize,
             info.compResult.K,
             info.compResult.BaseNum);

    return output;
}

void processTraceFile(Cache *cache, const char *filename, CompressionResult *compResult) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        perror("Failed to open file");
        exit(EXIT_FAILURE);
    }

    char *csvName = processTraceFileName(filename);

    FILE *csv = fopen(csvName, "w");
    if (csv == NULL) {
        perror("Unable to open file");
        exit(EXIT_FAILURE);
    }
    free(csvName);

    fprintf(csv, "MemAddress,ifHit,ifEvict,roundedCompSize,timestamp,isZero,isSame,compSize,K,baseNum\n");

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
            cachingByAddrAndRandomMemContent(cache, compResult, address, operation, csv);
            if(RP == CAMP){
                if(cache->CAMP_training_counter == 1){
                    CAMPWeightUpdate(cache);
                    cache->CAMP_training_counter = 160;
                } else {
                    cache->CAMP_training_counter -= 1;
                }
            }
        } else {
            fprintf(stderr, "Error parsing line: %s", line);
        }
    }

    fclose(file);
    fclose(csv);
}

char *processTraceFileName(const char *filename) {
    const char *prefix = "testTraces/";
    const char *suffix = ".trace";
    const char *outputDir = "testOutput/";

    const char *start = strstr(filename, prefix);
    if (start != NULL) {
        start += strlen(prefix);
    } else {
        start = filename;
    }

    const char *end = strstr(start, suffix);
    if (end == NULL) {
        end = start + strlen(start);
    }

    int nameLength = end - start;

    char *name = malloc(nameLength + 1);
    if (name == NULL) {
        perror("Failed to allocate memory");
        exit(EXIT_FAILURE);
    }
    strncpy(name, start, nameLength);
    name[nameLength] = '\0';

    char *newFilename = malloc(strlen(outputDir) + strlen(name) + 10 + 5);
    if (newFilename == NULL) {
        perror("Failed to allocate memory for new filename");
        free(name);
        exit(EXIT_FAILURE);
    }
    char *RPsuffix = "_lru";
    switch (RP)
    {
    case RANDOM:
        RPsuffix = "_random";
        break;
    case BESTFIT:
        RPsuffix = "_bestfit";
        break;
    case LRU:
        RPsuffix = "_lru";
        break;
    case CAMP:
        RPsuffix = "_camp";
        break;
    default:
        RPsuffix = "_lru";
        break;
    }
    sprintf(newFilename, "%s%s%s.csv", outputDir, name, RPsuffix);

    free(name);
    return newFilename;
}

//modified https://stackoverflow.com/questions/36714030/c-sort-float-array-while-keeping-track-of-indices
int cmp(const void *a, const void *b)
{
    arrayTuple *a1 = (arrayTuple *)a;
    arrayTuple *a2 = (arrayTuple *)b;
    if ((*a1).value > (*a2).value)
        return -1;
    else if ((*a1).value < (*a2).value)
        return 1;
    else
        return 0;
}

void CAMPWeightUpdate(Cache* cache){
    //printf("Entered Weight Update\n");
    arrayTuple result_array[8];
    for(int i = 0; i < 8; i++){
        result_array[i].value = 0;
        result_array[i].index = i;
    }
    //printf("initialized result_array\n");
    for(int j=0; j < NUMBER_OF_SETS; j++){
        for(int k = 0; k < 16; k++){
            CacheSet tmpSet;
            tmpSet = cache->sets[j];
            int history = tmpSet.CAMP_history_buffer[k] / 4;
            if(history > 0){
                //printf("j: %d, k:%d, history: %d\n", j, k, history);
                result_array[history-1].value += 1;
            }
            //printf("result_array new value: %d\n", result_array[history].value);
            cache->sets[j].CAMP_history_buffer[k] = 0;
            if(cache->sets[j].CAMP_history_buffer[k] != 0) printf("%d\n",cache->sets[j].CAMP_history_buffer[k]);
        }
        cache->sets[j].CAMP_hb_count = 0;
    }
    //printf("updated result_array\n");
    qsort(result_array, 8, sizeof(result_array[0]), cmp);
    //printf("sorted result_array\n");
    int NewWeightTable[8];
    for(int i = 0; i < 8; i++){
        NewWeightTable[result_array[7-i].index] = i + 1;
    }
    //printf("new weight table formed\n");
    for(int j = 0; j < NUMBER_OF_SETS; j++){
        for(int k = 0; k < 8; k++){
            cache->sets[j].CAMP_weight_table[k] = NewWeightTable[k];
        }
    }
    //printf("copied new weight table over\n");
    return;
}