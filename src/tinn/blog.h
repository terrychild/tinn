#ifndef TINN_BLOG_H
#define TINN_BLOG_H

#include "lib/types.h"
#include "lib/net/http.h"

typedef struct {
    Slice dir;
    Slice title;
    Slice date;
    Slice url;
    MappedFile* content;
} BlogPost;

typedef struct {
    Array* posts;
    Slice header1;
    Slice header2;
    Slice footer;
} Blog;

Blog* blogNew(Allocator* allocator);
bool blogContent(Blog* blog, HttpServerExchange* exchange);

#endif