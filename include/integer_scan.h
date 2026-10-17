#pragma once
// Only the reviewed integer/string formats used by DNSP checkpoints, console
// settings and the pinned coexistence/time-zone queries are supported. No floats,
// dynamic allocation, unbounded string conversion or global scanf replacement.
int dnspIntegerScan(const char* input, const char* format, ...);
#define DNSP_INTEGER_SCAN dnspIntegerScan
