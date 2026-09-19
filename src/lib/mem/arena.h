#ifndef LIB_MEM_ARENA_H
#define LIB_MEM_ARENA_H

#include "lib/types.h"

typedef struct ArenaStackFrame ArenaStackFrame;

typedef struct {
    U64 size;
    U64 committed;
    U64 allocated;
    U8* data;
    ArenaStackFrame* top;
} Arena;

struct ArenaStackFrame {
    ArenaStackFrame* next;
};

void arenaInit(Arena* arean, U64 size);
void arenaReset(Arena* arena);
void arenaRelease(Arena* arena);

void* arenaAlloc(Arena* arena, U64 size);
void* arenaAllocRaw(Arena* arena, U64 size);

void arenaPushFrame(Arena* arena);
void arenaPopFrame(Arena* arena);

#endif