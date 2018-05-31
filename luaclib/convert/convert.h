#ifndef CONVERT_H
#define CONVERT_H

#include <assert.h>
#include <math.h>
#include <string.h>

#if defined(_MSC_VER)
#include "stdint.h"
#include <intrin.h>
#else
#include <stdint.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif
char* i64toa_fast(int64_t value, char* buffer);
char* i32toa_fast(int32_t value, char* buffer);
void dtoa_fast(double value, char* buffer);
double strtod_fast(const char *str, int length,int* result);
#ifdef __cplusplus
};
#endif
#endif 
