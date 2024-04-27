BDI simulation

BDI source code: https://github.com/CMU-SAFARI/BDICompression.git

Trace files from: https://github.com/jiangxincode/CacheSim.git

Usage:
1. compressed cache simulation (32KB, 32-byte cacheline, 64-byte set, BDI compression):
  - make / make clean
  - ./cache
  - choose a trace file from testTraces folder (e.g. testTraces/gcc.trace)
  - choose replacement policy (RANDOM, BESTFIT, LRU)
2. check memory address range for a trace file:
  - gcc checkTrace.c -o checkTrace
  - ./checkTrace testTraces/xxx.trace
