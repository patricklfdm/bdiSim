/*
 * Original Code: https://github.com/CMU-SAFARI/BDICompression.git
 * 
 * Modified by Penggao Li
 * Last modified: 04/26/2024
 */

#include <stdlib.h>
#include <stdio.h>

#include "bdi.h"

EndianType type = BIG;

static unsigned long long my_llabs(long long x)
{
    unsigned long long t = x >> 63;
    return (x ^ t) - t;
}

static unsigned my_abs(int x)
{
    unsigned t = x >> 31;
    return (x ^ t) - t;
}

// long long unsigned *convertBuffer2Array(char *buffer, unsigned size, unsigned step)
// {
//     long long unsigned *values = (long long unsigned *)malloc(sizeof(long long unsigned) * size / step);

//     unsigned int i, j;
//     for (i = 0; i < size / step; i++)
//     {
//         values[i] = 0; // Initialize all elements to zero.
//     }
//     for (i = 0; i < size; i += step)
//     {
//         for (j = 0; j < step; j++)
//         {
//             values[i / step] += (long long unsigned)((unsigned char)buffer[i + j]) << (8 * j);
//         }
//     }
//     return values;
// }

unsigned long long readBytesAsInteger(const unsigned char *bytes, unsigned step, EndianType endianType)
{
    unsigned long long value = 0;
    if (endianType == LITTLE)
    {
        for (int j = 0; j < step; j++)
        {
            value += (unsigned long long)(bytes[j]) << (8 * j);
        }
    }
    else
    { // BIG_ENDIAN
        for (int j = 0; j < step; j++)
        {
            value += (unsigned long long)(bytes[j]) << (8 * (step - 1 - j));
        }
    }
    return value;
}

unsigned long long *convertBuffer2Array(char *buffer, unsigned size, unsigned step, EndianType endianType)
{
    unsigned long long *values = (unsigned long long *)malloc(sizeof(unsigned long long) * (size / step));
    if (!values)
    {
        fprintf(stderr, "Memory allocation failed\n");
        return NULL;
    }

    for (unsigned i = 0; i < size; i += step)
    {
        values[i / step] = readBytesAsInteger((unsigned char *)buffer + i, step, endianType);
    }

    return values;
}

///
/// Check if the cache line consists of only zero values
///
int isZeroPackable(long long unsigned *values, unsigned size)
{
    int nonZero = 0;
    unsigned int i;
    for (i = 0; i < size; i++)
    {
        if (values[i] != 0)
        {
            nonZero = 1;
            break;
        }
    }
    return !nonZero;
}

///
/// Check if the cache line consists of only same values
///
int isSameValuePackable(long long unsigned *values, unsigned size)
{
    int notSame = 0;
    unsigned int i;
    for (i = 0; i < size; i++)
    {
        if (values[0] != values[i])
        {
            notSame = 1;
            break;
        }
    }
    return !notSame;
}

///
/// Check if the cache line values can be compressed with multiple base + 1,2,or 4-byte offset
/// Returns size after compression
///
unsigned doubleExponentCompression(long long unsigned *values, unsigned size, unsigned blimit, unsigned bsize)
{
    unsigned long long limit = 0;
    // define the appropriate size for the mask
    switch (blimit)
    {
    case 1:
        limit = 56;
        break;
    case 2:
        limit = 48;
        break;
    default:
        exit(1);
    }
    // finding bases: # BASES
    // find how many elements can be compressed with mbases
    unsigned compCount = 0;
    unsigned int i;
    for (i = 0; i < size; i++)
    {
        if ((values[0] >> limit) == (values[i] >> limit))
        {
            compCount++;
        }
    }
    // return compressed size
    if (compCount != size)
        return size * bsize;
    return size * bsize - (compCount - 1) * blimit;
}

///
/// Check if the cache line values can be compressed with multiple base + 1,2,or 4-byte offset
/// Returns size after compression
/// 
CurrCompResult multBaseCompression(long long unsigned *values, unsigned size, unsigned blimit, unsigned bsize, unsigned base)
{
    CurrCompResult result;
    unsigned long long limit = 0;
    unsigned BASES = base;
    // define the appropriate size for the mask
    switch (blimit)
    {
    case 1:
        limit = 0xFF;
        break;
    case 2:
        limit = 0xFFFF; //
        break;
    case 4:
        limit = 0xFFFFFFFF;
        break;
    default:
        exit(1);
    }
    // finding bases: # BASES
    unsigned long long mbases[64];
    unsigned baseCount = 1;
    mbases[0] = 0;
    unsigned int i, j;
    for (i = 0; i < size; i++)
    {
        for (j = 0; j < baseCount; j++)
        {
            if (my_llabs((long long int)(mbases[j] - values[i])) > limit)
            {
                mbases[baseCount++] = values[i];
            }
        }
        if (baseCount >= BASES) // we don't have more bases
            break;
    }
    // printf("\nbaseCount: %d\n", baseCount);
    result.baseCount = baseCount;
    // find how many elements can be compressed with mbases
    unsigned compCount = 0;
    unsigned compBase1Count = 0;
    unsigned compBase2Count = 0;
    for (i = 0; i < size; i++)
    {
        // ol covered = 0;
        for (j = 0; j < baseCount; j++)
        {
            if (my_llabs((long long int)(mbases[j] - values[i])) <= limit)
            {
                compCount++;
                if (j == 0)
                {
                    compBase1Count++;
                }
                else if (j == 1)
                {
                    compBase2Count++;
                }
                break;
            }
        }
    }
    // printf("\n1: %d, 2: %d\n", compBase1Count, compBase2Count);

    // return compressed size
    // printf("\nblimit: %d, compCount: %d, bsize: %d, size: %d\n", blimit, compCount, bsize, size);
    // unsigned mCompSize = blimit * compCount + bsize * BASES + (size - compCount) * bsize;
    unsigned mCompSize = baseCount * bsize + compBase1Count + compBase2Count * 2;
    // printf("mCompSize: %d, size: %d\n", mCompSize, size);
    if (compCount < size)
    {
        result.compSize = size * bsize;
        return result;
    }
    result.compSize = mCompSize;
    return result;
}

void printValuesArr(long long unsigned *values, unsigned _blockSize, unsigned step)
{
    if (values)
    {
        printf("\n========================\n");
        printf("Step: %d-byte\n", step);
        for (int i = 0; i < _blockSize / step; i++)
        {
            printf("Value %d: 0x%llX\n", i, values[i]);
        }
        printf("========================\n");
    }
}

// unsigned BDICompress(char *buffer, unsigned _blockSize)
// {
//     long long unsigned *values = NULL;
//     unsigned bestCSize = 0;
//     unsigned currCSize = 0;

//     values = convertBuffer2Array(buffer, _blockSize, 8, type);
//     printValuesArr(values, _blockSize, 8);
//     bestCSize = _blockSize;
//     currCSize = _blockSize;
//     if (isZeroPackable(values, _blockSize / 8))
//         bestCSize = 1;
//     if (isSameValuePackable(values, _blockSize / 8))
//         currCSize = 8;
//     bestCSize = bestCSize > currCSize ? currCSize : bestCSize;
//     currCSize = multBaseCompression(values, _blockSize / 8, 1, 8, 2);
//     bestCSize = bestCSize > currCSize ? currCSize : bestCSize;
//     currCSize = multBaseCompression(values, _blockSize / 8, 2, 8, 2);
//     bestCSize = bestCSize > currCSize ? currCSize : bestCSize;
//     currCSize = multBaseCompression(values, _blockSize / 8, 4, 8, 2);
//     bestCSize = bestCSize > currCSize ? currCSize : bestCSize;
//     free(values);

//     //===================================================================
//     values = convertBuffer2Array(buffer, _blockSize, 4, type);
//     printValuesArr(values, _blockSize, 4);
//     if (isSameValuePackable(values, _blockSize / 4))
//         currCSize = 4;
//     bestCSize = bestCSize > currCSize ? currCSize : bestCSize;
//     currCSize = multBaseCompression(values, _blockSize / 4, 1, 4, 2);
//     printf("\n%d", currCSize);
//     bestCSize = bestCSize > currCSize ? currCSize : bestCSize;
//     currCSize = multBaseCompression(values, _blockSize / 4, 2, 4, 2);
//     printf("\n%d\n", currCSize);
//     bestCSize = bestCSize > currCSize ? currCSize : bestCSize;
//     free(values);

//     //===================================================================
//     values = convertBuffer2Array(buffer, _blockSize, 2, type);
//     printValuesArr(values, _blockSize, 2);
//     if (isSameValuePackable(values, _blockSize / 2))
//         currCSize = 2;
//     bestCSize = bestCSize > currCSize ? currCSize : bestCSize;
//     currCSize = multBaseCompression(values, _blockSize / 2, 1, 2, 2);
//     bestCSize = bestCSize > currCSize ? currCSize : bestCSize;
//     free(values);

//     // delete [] buffer;
//     buffer = NULL;
//     values = NULL;
//     return bestCSize;
// }

CompressionResult setCompResult(unsigned isZero, unsigned isSame, unsigned compSize, unsigned K, unsigned baseNum)
{
    CompressionResult result;
    result.isZero = isZero;
    result.isSame = isSame;
    result.compSize = compSize;
    result.K = K;
    result.BaseNum = baseNum;
    return result;
}

CompressionResult BDICompress(char *buffer, unsigned _blockSize)
{
    long long unsigned *values = NULL;
    unsigned bestCSize = 0;
    unsigned currCSize = 0;

    CompressionResult result = {0, 0, _blockSize, 0, 0};
    CurrCompResult currResult = {0, _blockSize};

    values = convertBuffer2Array(buffer, _blockSize, 8, type);
    printValuesArr(values, _blockSize, 8);
    bestCSize = _blockSize;
    currCSize = _blockSize;
    if (isZeroPackable(values, _blockSize / 8))
    {
        // bestCSize = 1;
        result = setCompResult(1, 1, 1, 1, 0);
        free(values);
        return result;
    }
    if (isSameValuePackable(values, _blockSize / 8))
    {
        currCSize = 8;
        bestCSize = bestCSize > currCSize ? currCSize : bestCSize;
        result = setCompResult(0, 1, 8, 8, 1);
    }
    else
    {
        currResult = multBaseCompression(values, _blockSize / 8, 1, 8, 2);
        currCSize = currResult.compSize;
        if (currCSize < bestCSize)
        {
            bestCSize = currCSize;
            result = setCompResult(0, 0, bestCSize, 8, currResult.baseCount);
        }
        currResult = multBaseCompression(values, _blockSize / 8, 2, 8, 2);
        currCSize = currResult.compSize;
        if (currCSize < bestCSize)
        {
            bestCSize = currCSize;
            result = setCompResult(0, 0, bestCSize, 8, currResult.baseCount);
        }
        currResult = multBaseCompression(values, _blockSize / 8, 4, 8, 2);
        currCSize = currResult.compSize;
        if (currCSize < bestCSize)
        {
            bestCSize = currCSize;
            result = setCompResult(0, 0, bestCSize, 8, currResult.baseCount);
        }
    }
    free(values);

    //===================================================================
    values = convertBuffer2Array(buffer, _blockSize, 4, type);
    printValuesArr(values, _blockSize, 4);
    if (isSameValuePackable(values, _blockSize / 4))
    {
        currCSize = 4;
        bestCSize = bestCSize > currCSize ? currCSize : bestCSize;
        result = setCompResult(0, 1, 4, 4, 1);
    }
    else
    {
        currResult = multBaseCompression(values, _blockSize / 4, 1, 4, 2);
        currCSize = currResult.compSize;
        // printf("\n%d", currCSize);
        if (currCSize < bestCSize)
        {
            bestCSize = currCSize;
            result = setCompResult(0, 0, bestCSize, 4, currResult.baseCount);
        }
        currResult = multBaseCompression(values, _blockSize / 4, 2, 4, 2);
        currCSize = currResult.compSize;
        // printf("\n%d\n", currCSize);
        if (currCSize < bestCSize)
        {
            bestCSize = currCSize;
            result = setCompResult(0, 0, bestCSize, 4, currResult.baseCount);
        }
    }
    free(values);

    //===================================================================
    values = convertBuffer2Array(buffer, _blockSize, 2, type);
    printValuesArr(values, _blockSize, 2);
    if (isSameValuePackable(values, _blockSize / 2))
    {
        result = setCompResult(0, 1, 2, 2, 1);
        free(values);
        return result;
    }
    else
    {
        currResult = multBaseCompression(values, _blockSize / 2, 1, 2, 2);
        currCSize = currResult.compSize;
        if (currCSize < bestCSize)
        {
            bestCSize = currCSize;
            result = setCompResult(0, 0, bestCSize, 2, currResult.baseCount);
        }
    }
    free(values);

    // delete [] buffer;
    buffer = NULL;
    values = NULL;
    // printf("\ncompSize: %d\n", result.compSize);
    return result;
}

unsigned FPCCompress(char *buffer, unsigned size)
{
    long long unsigned *values = convertBuffer2Array(buffer, size * 4, 4, type);
    unsigned compressable = 0;
    unsigned int i;
    for (i = 0; i < size; i++)
    {
        //  000
        if (values[i] == 0)
        {
            compressable += 1;
            continue;
        }
        // 001 010
        if (my_abs((int)(values[i])) <= 0xFF)
        {
            compressable += 1;
            continue;
        }
        // 011
        if (my_abs((int)(values[i])) <= 0xFFFF)
        {
            compressable += 2;
            continue;
        }
        // 100
        if (((values[i]) & 0xFFFF) == 0)
        {
            compressable += 2;
            continue;
        }
        // 101
        if (my_abs((int)((values[i]) & 0xFFFF)) <= 0xFF && my_abs((int)((values[i] >> 16) & 0xFFFF)) <= 0xFF)
        {
            compressable += 2;
            continue;
        }
        // 110
        unsigned byte0 = (values[i]) & 0xFF;
        unsigned byte1 = (values[i] >> 8) & 0xFF;
        unsigned byte2 = (values[i] >> 16) & 0xFF;
        unsigned byte3 = (values[i] >> 24) & 0xFF;
        if (byte0 == byte1 && byte0 == byte2 && byte0 == byte3)
        {
            compressable += 1;
            continue;
        }
        // 111
        compressable += 4;
    }
    free(values);
    // 6 bytes for 3 bit per every 4-byte word in a 64 byte cache line
    unsigned compSize = compressable + size * 3 / 8;
    if (compSize < size * 4)
        return compSize;
    else
        return size * 4;
}

// unsigned GeneralCompress(char *buffer, unsigned _blockSize, unsigned compress)
// { // compress is the actual compression algorithm
//     switch (compress)
//     {
//     case 0:
//         return _blockSize;
//         break;
//     case 1:
//         return BDICompress(buffer, _blockSize);
//         break;
//     case 2:
//         return FPCCompress(buffer, _blockSize / 4);
//         break;
//     case 3:
//     {
//         unsigned BDISize = BDICompress(buffer, _blockSize);
//         unsigned FPCSize = FPCCompress(buffer, _blockSize / 4);
//         if (BDISize <= FPCSize)
//         {
//             printf("Use BDI: %d (FPC: %d)\n", BDISize, FPCSize);
//             return BDISize;
//         }
//         else
//         {
//             printf("Use FPC: %d (BDI: %d)\n", FPCSize, BDISize);
//             return FPCSize;
//         }
//         break;
//     }
//     default:
//         exit(1);
//     }
// }

void printBuffer(const char *buffer, unsigned size)
{
    printf("Buffer contents (hexadecimal, %u bytes):\n", size);
    for (unsigned i = 0; i < size; ++i)
    {
        printf("%02x ", (unsigned char)buffer[i]);
        if ((i + 1) % 4 == 0) // 4 bytes
        {
            printf("\n");
        }
    }
    printf("\n");
}

// Convert the binary string to an unsigned long integer
unsigned long binaryStringToHex(const char *binaryString)
{
    unsigned long intValue = strtoul(binaryString, NULL, 2);
    return intValue;
}

BufferStruct readHexValuesIntoBuffer(const char *filename)
{
    FILE *file = fopen(filename, "r");
    BufferStruct result = {NULL, 0};
    size_t numberOfHexValues = 0;

    if (file == NULL)
    {
        fprintf(stderr, "Error opening file.\n");
        return result;
    }

    // Count the number of hex values in the file
    char line[256];
    while (fgets(line, sizeof(line), file))
    {
        numberOfHexValues++;
    }

    // Calculate the buffer size in bytes
    result.size = numberOfHexValues * 4;

    // Allocate the buffer to hold all the values
    result.buffer = (unsigned char *)malloc(result.size);
    if (result.buffer == NULL)
    {
        fprintf(stderr, "Memory allocation failed.\n");
        result.size = 0;
        fclose(file);
        return result;
    }

    // Reset file pointer to start of the file and read the hex values
    fseek(file, 0, SEEK_SET);
    size_t index = 0;
    while (fgets(line, sizeof(line), file))
    {
        // Read hex value
        unsigned long value = strtoul(line, NULL, 16);
        // Store the value as 4 bytes in the buffer
        result.buffer[index++] = (value >> 24) & 0xFF;
        result.buffer[index++] = (value >> 16) & 0xFF;
        result.buffer[index++] = (value >> 8) & 0xFF;
        result.buffer[index++] = value & 0xFF;
    }

    fclose(file);
    return result;
}

int main()
{
    const char *filename = "testHex/hex1.txt";
    BufferStruct bufferStruct = readHexValuesIntoBuffer(filename);

    if (bufferStruct.buffer == NULL)
    {
        perror("Failed to read buffer from file.");
        exit(EXIT_FAILURE);
    }

    unsigned bufferSize = bufferStruct.size;
    unsigned blockSize = 4;
    char *buffer = bufferStruct.buffer;

    if (!buffer)
    {
        fprintf(stderr, "Failed to allocate memory for buffer\n");
        return EXIT_FAILURE;
    }

    printf("Buffer read with size: %zu bytes\n", bufferStruct.size);
    printBuffer(bufferStruct.buffer, bufferStruct.size);

    // Call the BDICompress function
    // unsigned compressedSize = GeneralCompress(buffer, bufferSize, 3);
    CompressionResult compResult = BDICompress(buffer, bufferSize);

    // Check the result
    if (compResult.compSize == 0)
    {
        printf("Compression failed or resulted in no size reduction.\n");
    }
    else
    {
        printf("Original Size: %u, Compressed Size: %u\n", bufferSize, compResult.compSize);
        printf("isZero: %d, isSame: %d, K: %d, baseNum: %d\n", compResult.isZero, compResult.isSame, compResult.K, compResult.BaseNum);
    }

    free(bufferStruct.buffer);

    return EXIT_SUCCESS;
}