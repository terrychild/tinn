#ifndef LIB_BIT_H
#define LIB_BIT_H

#include <stdint.h>

uint8_t rotl8(uint8_t value, unsigned int count);
uint32_t rotl32(uint32_t value, unsigned int count);

#define rotl(value, count) _Generic((value),	\
		uint32_t: rotl32 \
		default: rotl8	\
	)(value, count)

#endif