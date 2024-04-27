BDI simulation

BDI source code: https://github.com/CMU-SAFARI/BDICompression.git

Trace files from: https://github.com/jiangxincode/CacheSim.git

Usage:
1. compressed cache simulation (32KB, 32-byte cacheline, 64-byte set):
  - make
  - ./cache
  - choose trace file from testTraces folder
  - choose replacement policy (RANDOM, BESTFIT, LRU)
  - make clean
2. check memory address range for a trace file:
  - gcc checkTrace.c -o checkTrace
  - ./checkTrace testTraces/xxx.trace
