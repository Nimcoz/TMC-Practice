#ifndef PRACTICE_TYPES_H
#define PRACTICE_TYPES_H

#include <stdint.h>
#include <stddef.h>

typedef uint8_t u8;
typedef int8_t s8;
typedef uint16_t u16;
typedef int16_t s16;
typedef uint32_t u32;
typedef int32_t s32;
typedef uintptr_t uptr;

#define ARRAY_COUNT(array) (sizeof(array) / sizeof((array)[0]))

#endif
