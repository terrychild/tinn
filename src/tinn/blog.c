#include "blog.h"
#include "lib/log.h"
#include "lib/macros.h"
#include "lib/mem/allocator.h"
#include "lib/mem/file.h"
#include "lib/mem/array.h"
#include "lib/mem/buffer.h"
#include "lib/mem/slice.h"

Blog* blogNew(Allocator* allocator) {
    Blog* blog = allocate(allocator, sizeof(Blog));
    blog->posts = arrayNew(allocator, sizeof(BlogPost), 32);

    File posts = fileRead("./blog/.posts.dat");
    if (!posts.start) {
        ERROR("Unable to read load posts.dat");
        return NULL;
    }
    Tokeniser lines = sliceTokeniserStr(fileAsSlice(posts), "\n", false);
    Slice line = nextToken(&lines);
    while (line.start && line.length > 0) {
        Tokeniser fields = sliceTokeniserStr(line, "\t", true);
        BlogPost* post = arrayAdd(blog->posts);
        post->path = nextToken(&fields);
        post->title = nextToken(&fields);
        post->date = nextToken(&fields);
        line = nextToken(&lines);
    }

    return blog;
}

bool blogContent(Blog* blog, HttpServerExchange* exchange) {
    Buffer* content = bufNew(exchange->scope, KB(4));

    bufAppendStr(content, "Posts\n=====\n");
    for (U64 i=0; i<blog->posts->count; i++) {
        BlogPost* post = (BlogPost*)arrayGet(blog->posts, i);
        bufAppendSlice(content, post->date);
        bufAppendStr(content, " -- ");
        bufAppendSlice(content, post->title);
        bufAppendStr(content, " -- ");
        bufAppendSlice(content, post->path);
        bufAppendStr(content, "\n");
    }

    httpServerSetContent(exchange, sliceFromStr("txt"), bufAsSlice(content));
    httpServerSend(exchange);
    return true;
}