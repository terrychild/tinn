#ifndef LIB_MEM_ARENA_H
#define LIB_MEM_ARENA_H

#include "lib/types.h"

typedef struct Arena Arena;
typedef struct ArenaStackFrame ArenaStackFrame;
typedef struct ArenaChildArena ArenaChildArena;

struct Arena {
    U64 size;
    U64 committed;
    U64 allocated;
    U8* data;
    ArenaStackFrame* top;
};

struct ArenaStackFrame {
    ArenaStackFrame* next;
    ArenaChildArena* child;
};

struct ArenaChildArena {
    ArenaChildArena* next;
    Arena* arena;
};

Arena* arenaNew(U64 size, bool stackable);
void arenaInit(Arena* arean, U64 size, bool stackable);
void arenaReset(Arena* arena);
void arenaRelease(Arena* arena);

void* arenaAlloc(Arena* arena, U64 size);
void* arenaAllocRaw(Arena* arena, U64 size);

Arena* arenaAddChild(Arena* arena, U64 size, bool stackable);

void arenaPushFrame(Arena* arena);
void arenaPopFrame(Arena* arena);

#endif