BDI simulation
Original code: https://github.com/CMU-SAFARI/BDICompression.git

Usage:
1. test bdi compression:
  - modify the test file name "hex1.txt" source code based on the test
  - gcc bdi.c -o bdi
  - ./bdi
2. check memory address range for a trace file:
  - gcc checkTrace.c -o checkTrace
  - ./checkTrace testTraces/xxx.trace
