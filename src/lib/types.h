#ifndef LIB_TYPES
#define LIB_TYPES

#include <stddef.h>
#include <stdint.h>

typedef int8_t I8;
typedef int16_t I16;
typedef int32_t I32;
typedef int64_t I64;

typedef uint8_t U8;
typedef uint16_t U16;
typedef uint32_t U32;
typedef uint64_t U64;

typedef unsigned int uint;
typedef size_t Size;

typedef struct BigInt BigInt;

typedef struct Arena Arena;
typedef struct MappedFile MappedFile;
typedef struct Memory Memory;
typedef struct Array Array;
typedef struct Pool Pool;
typedef struct Buffer Buffer;

typedef struct {
    Size length;
    const U8* start;
} Slice;

#endif