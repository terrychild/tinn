#ifndef LIB_NET_URL_H
#define LIB_NET_URL_H

#include "lib/types.h"

typedef struct {
    //Slice scheme;
    //Slice host;
    //Slice port;
    Slice path;
    Array* path_segments;
    Slice query;
    //Slice fragment;
} URL;

URL* urlFromOrigin(Allocator* allocator, Slice origin);

#endif