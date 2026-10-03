#ifndef TINN_BLOG_H
#define TINN_BLOG_H

#include "lib/types.h"
#include "lib/net/http.h"

typedef struct {
    Slice path;
    Slice title;
    Slice date;
} BlogPost;

typedef struct {
    Array* posts;
} Blog;

Blog* blogNew(Allocator* allocator);
bool blogContent(Blog* blog, HttpServerExchange* exchange);

#endif