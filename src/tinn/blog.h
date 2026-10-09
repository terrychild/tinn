#ifndef TINN_BLOG_H
#define TINN_BLOG_H

#include <time.h>

#include "lib/types.h"
#include "lib/sys/polling.h"
#include "lib/net/http.h"

typedef struct {
    Slice dir;
    Slice title;
    Slice date;
    Slice url;
    MappedFile* content;
} BlogPost;

typedef struct {
    Allocator* allocator;
    Polling* polling;
    time_t mod_date;
    MappedFile* posts_file;
    Array* posts;
    MappedFile* header1;
    MappedFile* header2;
    MappedFile* footer;
} Blog;

Blog* blogNew(Allocator* allocator, Polling* polling);
bool blogContent(Blog* blog, HttpServerExchange* exchange);

#endif