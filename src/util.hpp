#pragma once
#include <cstdint>
#include <time.h>

#define S_IN_NS (1e+9)

uint32_t crc32_func(uint32_t crc, const uint8_t *buf, uint32_t size);
double diff_timespec(struct timespec *t1, struct timespec *t2);