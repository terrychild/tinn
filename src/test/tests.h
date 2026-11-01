#ifndef TEST_TESTS_H
#define TEST_TESTS_H

#include "lib/mem/memory.h"

void testThings();
void testTypes();
void testArena();
void testMemory(Memory* mem);
void testArray(Memory* mem);
void testPool(Memory* mem);
void testBuffer(Memory* mem);
void testMaths(Memory* mem);
void testCrypto(Memory* mem);

#endif