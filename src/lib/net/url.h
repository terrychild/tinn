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
	bool valid;
} URL;

URL urlParseOrigin(Allocator* allocator, Slice origin);

#endif