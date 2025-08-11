#ifndef LIB_MACROS_H
#define LIB_MACROS_H

#define STRINGIZER(x) #x
#define STR(x) STRINGIZER(x)

#define KB(x) (u64) (x * 1024)
#define MB(x) (u64) (x * 1024 * 1024)
#define GB(x) (u64) (x * 1024 * 1024 * 1024)

#endif