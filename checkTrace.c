/*
 * checkTrace.c
 * 
 * Created by Penggao Li
 * Last modified: 04/26/2024
 */

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <tracefile>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *filename = argv[1];
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Error opening file");
        return EXIT_FAILURE;
    }

    unsigned long long minAddr = ULLONG_MAX;
    unsigned long long maxAddr = 0;
    char line[100];
    unsigned long long addr;

    while (fgets(line, sizeof(line), file)) {
        if (sscanf(line, "%*c 0x%llx", &addr) == 1) {
            if (addr < minAddr) {
                minAddr = addr;
            }
            if (addr > maxAddr) {
                maxAddr = addr;
            }
        }
    }

    fclose(file);

    printf("Memory access range: 0x%llx to 0x%llx\n", minAddr, maxAddr);
    if (maxAddr > minAddr) {
        unsigned long long difference = maxAddr - minAddr;
        printf("Difference between max and min addresses: 0x%llx (%llu bytes)\n", difference, difference);
    } else {
        printf("No valid range could be determined.\n");
    }

    return EXIT_SUCCESS;
}
