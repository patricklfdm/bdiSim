/*
 * Original source code: https://github.com/CMU-SAFARI/BDICompression.git
 * 
 * Created by Penggao Li
 * Last modified: 04/27/2024
 */

#ifndef _BDI_H_
#define _BDI_H_

#include <stdlib.h>
#include <stdio.h>

typedef struct {
    unsigned char *buffer;
    size_t size;
} BufferStruct;

typedef enum {
    LITTLE,
    BIG
} EndianType;

typedef enum {
    BDI,
    FPC,
    NONE
} CompressedMethod;

typedef struct{
    unsigned baseCount;
    unsigned compSize;
}CurrCompResult;

// use bdi only
typedef struct {
    unsigned isZero;
    unsigned isSame;
    unsigned compSize;
    unsigned K;
    unsigned BaseNum; 
}CompressionResult;

unsigned long long my_llabs(long long x);

unsigned my_abs(int x);

unsigned long long readBytesAsInteger(const unsigned char *bytes, unsigned step, EndianType endianType);

// long long unsigned *convertBuffer2Array(char *buffer, unsigned size, unsigned step);
unsigned long long *convertBuffer2Array(unsigned char *buffer, unsigned size, unsigned step, EndianType endianType);

///
/// Check if the cache line consists of only zero values
///
int isZeroPackable(long long unsigned *values, unsigned size);

///
/// Check if the cache line consists of only same values
///
int isSameValuePackable(long long unsigned *values, unsigned size);

///
/// Check if the cache line values can be compressed with multiple base + 1,2,or 4-byte offset
/// Returns size after compression
///
unsigned doubleExponentCompression(long long unsigned *values, unsigned size, unsigned blimit, unsigned bsize);

///
/// Check if the cache line values can be compressed with multiple base + 1,2,or 4-byte offset
/// Returns size after compression
///
// unsigned multBaseCompression(long long unsigned *values, unsigned size, unsigned blimit, unsigned bsize);
CurrCompResult multBaseCompression(long long unsigned *values, unsigned size, unsigned blimit, unsigned bsize, unsigned base);

// unsigned BDICompress(char *buffer, unsigned _blockSize);
CompressionResult BDICompress(unsigned char *buffer, unsigned _blockSize);

unsigned FPCCompress(unsigned char *buffer, unsigned size);

// unsigned GeneralCompress(char *buffer, unsigned _blockSize, unsigned compress);

BufferStruct readHexValuesIntoBuffer(const char *filename);

void generateCompressedData(const char *filename, CompressionResult *compResult);

#endif