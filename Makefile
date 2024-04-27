# ============================================
#  Makefile for bdiSim
# 
#  Modified by Penggao Li
#  Last modified: 04/27/2024
# ============================================

OBJS	= main.o bdi.o compressedCache.o
SOURCE	= main.c bdi.c compressedCache.c
HEADER	= bdi.h compressedCache.h
OUT	= cache
FLAGS	= -g -c -Wall
LFLAGS	= 
CC	= gcc

all:	cache

cache: $(OBJS)
	$(CC) -g $(OBJS) -o $(OUT) $(LFLAGS)

main.o: main.c compressedCache.h
	$(CC) $(FLAGS) main.c

bdi.o: bdi.c
	$(CC) $(FLAGS) bdi.c 

compressedCache.o: compressedCache.c bdi.h
	$(CC) $(FLAGS) compressedCache.c

clean:
	rm -f $(OBJS) $(OUT)
