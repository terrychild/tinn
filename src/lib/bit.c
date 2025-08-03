#include <limits.h>

#include "bit.h"

uint8_t rotl8(uint8_t value, unsigned int count) {
	const unsigned int mask = CHAR_BIT * sizeof(value) - 1;
	count &= mask;
	return (value << count) | (value >> (-count & mask));
}

uint32_t rotl32(uint32_t value, unsigned int count) {
	const unsigned int mask = CHAR_BIT * sizeof(value) - 1;
	count &= mask;
	return (value << count) | (value >> (-count & mask));
}