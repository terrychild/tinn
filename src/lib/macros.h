#ifndef LIB_MACROS_H
#define LIB_MACROS_H

#define STRINGIZER(x) #x
#define STR(x) STRINGIZER(x)

#define KB(x) (U64) (x * 1024)
#define MB(x) (U64) (x * 1024 * 1024)
#define GB(x) (U64) (x * 1024 * 1024 * 1024)

#endif