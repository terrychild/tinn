#include "lib/bits.h"

U8 rotl8(U8 value, U64 count) {
	const U64 mask = 8 * sizeof(value) - 1;
	count &= mask;
	return (value << count) | (value >> (-count & mask));
}

U32 rotl32(U32 value, U64 count) {
	const U64 mask = 8 * sizeof(value) - 1;
	count &= mask;
	return (value << count) | (value >> (-count & mask));
}

U32 rotr32(U32 value, U64 count) {
	const U64 mask = 8 * sizeof(value) - 1;
	count &= mask;
	return (value >> count) | (value << (-count & mask));
}