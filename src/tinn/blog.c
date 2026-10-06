#include "blog.h"
#include "lib/log.h"
#include "lib/macros.h"
#include "lib/mem/allocator.h"
#include "lib/mem/mfile.h"
#include "lib/mem/array.h"
#include "lib/mem/buffer.h"
#include "lib/mem/slice.h"

Blog* blogNew(Allocator* allocator) {
    Blog* blog = allocate(allocator, sizeof(Blog));
    blog->posts = arrayNew(allocator, sizeof(BlogPost), 32);

    MappedFile* posts = allocateFile(allocator, "./blog/.posts.dat");
    if (!posts) {
        ERROR("Unable to load posts.dat");
        return NULL;
    }
    Tokeniser lines = sliceTokeniserStr(mfileAsSlice(posts), "\n", false);
    Slice line = nextToken(&lines);
    while (line.start && line.length > 0) {
        Tokeniser fields = sliceTokeniserStr(line, "\t", true);

        BlogPost* post = arrayAdd(blog->posts);
        post->dir = nextToken(&fields);
        post->title = nextToken(&fields);
        post->date = nextToken(&fields);

        Buffer* temp_path = bufNew(allocator, 32);
        bufAppendStr(temp_path, "./blog/");
        bufAppendSlice(temp_path, post->dir);
        post->url = bufSlice(temp_path, 1, -1);

        bufAppendStr(temp_path, "/.post.html");
        post->content = allocateFile(allocator, bufAsStr(temp_path));

        line = nextToken(&lines);
    }

    MappedFile* header1 = allocateFile(allocator, "./blog/.header1.html");
    MappedFile* header2 = allocateFile(allocator, "./blog/.header2.html");
    MappedFile* footer = allocateFile(allocator, "./blog/.footer.html");
    if (!header1 || !header2 || !footer) {
        ERROR("Unable to load fragment");
        deallocateFile(allocator, header1);
        deallocateFile(allocator, header2);
        deallocateFile(allocator, footer);
        return NULL;
    }
    blog->header1 = mfileAsSlice(header1);
    blog->header2 = mfileAsSlice(header2);
    blog->footer = mfileAsSlice(footer);

    return blog;
}

static bool isGetOrHead(HttpServerExchange* exchange) {
	if (!sliceIsStr(exchange->request->method, "GET") && !sliceIsStr(exchange->request->method, "HEAD")) {
        DEBUG("Method not allowed");
        httpServerAddHeader(exchange, sliceFromStr("Allow"), sliceFromStr("GET, HEAD"));
        httpServerSendError(exchange, HTTP_METHOD_NOT_ALLOWED);
        return false;
    }
    return true;
}

static void composeArticle(Buffer* buf, BlogPost* post) {
	bufAppendStr(buf, "<article>");
	bufAppendFormat(buf, "<h1><a href=\"%.*s\">%.*s</a></h1>", post->url.length, post->url.start, post->title.length, post->title.start);
	bufAppendFormat(buf, "<h2>%.*s</h2>", post->date.length, post->date.start);
	bufAppendSlice(buf, mfileAsSlice(post->content));
	bufAppendStr(buf, "</article>\n");
}

bool blogContent(Blog* blog, HttpServerExchange* exchange) {
    Buffer* content = bufNew(exchange->scope, KB(4));

    URL* target = exchange->request->target;

    // home page
    if (sliceIsStr(target->path, "/")) {
        if (!isGetOrHead(exchange)) {
            return true;
        }

        // generate content
		bufAppendSlice(content, blog->header1);
        bufAppendSlice(content, blog->header2);
        for (U64 i=0; i<blog->posts->count; i++) {
            if (i > 0) {
				bufAppendStr(content, "<hr>\n");
			}
            composeArticle(content, (BlogPost*)arrayGet(blog->posts, i));
        }
		bufAppendSlice(content, blog->footer);

        // send
        httpServerAddHeader(exchange, sliceFromStr("Cache-Control"), sliceFromStr("no-cache"));
		//httpServerAddDateHeader(exchange, "Last-Modified", mod_date);
		httpServerSetContent(exchange, sliceFromStr("html"), bufAsSlice(content));
        httpServerSend(exchange);
        return true;
    }

    // log page
    if (target->path_segments->count == 1 && sliceIsStr(*(Slice*)arrayGet(target->path_segments, 0), "log")) {
        if (!isGetOrHead(exchange)) {
            return true;
        }

        // generate content
		bufAppendSlice(content, blog->header1);
        bufAppendSlice(content, blog->header2);
        U64 i = blog->posts->count;
        if (i>0) {
            do {
                i--;
                if (i < blog->posts->count - 1) {
                    bufAppendStr(content, "<hr>\n");
                }
                composeArticle(content, (BlogPost*)arrayGet(blog->posts, i));
            } while (i>0);
        }
        for (U64 i=0; i<blog->posts->count; i++) {
            if (i > 0) {
				bufAppendStr(content, "<hr>\n");
			}
            composeArticle(content, (BlogPost*)arrayGet(blog->posts, i));
        }
		bufAppendSlice(content, blog->footer);

        // send
        httpServerAddHeader(exchange, sliceFromStr("Cache-Control"), sliceFromStr("no-cache"));
		//httpServerAddDateHeader(exchange, "Last-Modified", mod_date);
		httpServerSetContent(exchange, sliceFromStr("html"), bufAsSlice(content));
        httpServerSend(exchange);
        return true;
    }

    // blog
    if (target->path_segments->count >= 1 && sliceIsStr(*(Slice*)arrayGet(target->path_segments, 0), "blog")) {
        if (target->path_segments->count == 1) {
            if (!isGetOrHead(exchange)) {
                return true;
            }

            // generate content
            bufAppendSlice(content, blog->header1);
            bufAppendStr(content, " - Blog");
            bufAppendSlice(content, blog->header2);
            bufAppendStr(content, "<article><h1>Blog Archive</h1>\n");
		    bufAppendStr(content, "<p>If you, like me, sometimes want to read an entire blog in chronological order without any unnecessary navigating and/or scrolling back and forth, you can do that <a href=\"/log\">here</a>.</p>\n");

            Slice last_date = sliceEmpty();
            for (U64 i=0; i<blog->posts->count; i++) {
                BlogPost* post = (BlogPost*)arrayGet(blog->posts, i);
                Slice date = sliceRightStr(post->date, " ");
                if (!sliceIs(last_date, date)) {
                    bufAppendFormat(content, "<hr>\n<h3>%.*s</h3>\n", date.length, date.start);
                    last_date = date;
                }
                bufAppendFormat(content, "<p><a href=\"%.*s\">%.*s</a></p>\n", post->url.length, post->url.start, post->title.length, post->title.start);
            }

            bufAppendStr(content, "</article>");
		    bufAppendSlice(content, blog->footer);

            // send
            httpServerAddHeader(exchange, sliceFromStr("Cache-Control"), sliceFromStr("no-cache"));
            //httpServerAddDateHeader(exchange, "Last-Modified", mod_date);
            httpServerSetContent(exchange, sliceFromStr("html"), bufAsSlice(content));
            httpServerSend(exchange);
            return true;
        }

        // blog post
        if (target->path_segments->count == 2) {
            for (U64 i=0; i<blog->posts->count; i++) {
                BlogPost* post = (BlogPost*)arrayGet(blog->posts, i);
                if (sliceIs(*(Slice*)arrayGet(target->path_segments, 1), post->dir)) {
                    // generate content
                    bufAppendSlice(content, blog->header1);
                    bufAppendFormat(content, " - %.*s", post->title.length, post->title.start);
                    bufAppendSlice(content, blog->header2);
                    bufAppendFormat(content, "<article><h1>%.*s</h1><h2>%.*s</h2>\n", post->title.length, post->title.start, post->date.length, post->date.start);
                    bufAppendSlice(content, mfileAsSlice(post->content));
                    bufAppendStr(content, "<nav>");
                    if (i < blog->posts->count - 1) {
                        BlogPost* other_post = (BlogPost*)arrayGet(blog->posts, i + 1);
                        bufAppendFormat(content, "<a href=\"%.*s\">previous</a>", other_post->url.length, other_post->url.start);
                    } else {
                        bufAppendStr(content, "<span>&nbsp;</span>");
                    }
                    if (i > 0) {
                        BlogPost* other_post = (BlogPost*)arrayGet(blog->posts, i - 1);
                        bufAppendFormat(content, "<a href=\"%.*s\">next</a>", other_post->url.length, other_post->url.start);
                    }
                    bufAppendStr(content, "</nav></article>");
		            bufAppendSlice(content, blog->footer);

                    // send
                    httpServerAddHeader(exchange, sliceFromStr("Cache-Control"), sliceFromStr("no-cache"));
                    //httpServerAddDateHeader(exchange, "Last-Modified", mod_date);
                    httpServerSetContent(exchange, sliceFromStr("html"), bufAsSlice(content));
                    httpServerSend(exchange);
                    return true;
                }
            }
        }
    }

    return false;
}