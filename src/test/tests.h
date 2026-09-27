#ifndef TEST_TESTS_H
#define TEST_TESTS_H

#include "lib/mem/allocator.h"

void testTypes();
void testArena();
void testAllocator(Allocator* allocator);
void testArray(Allocator* allocator);
void testPool(Allocator* allocator);
void testBuffer(Allocator* allocator);

#endif