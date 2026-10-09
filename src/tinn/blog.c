#include "blog.h"
#include "lib/log.h"
#include "lib/macros.h"
#include "lib/mem/allocator.h"
#include "lib/mem/mfile.h"
#include "lib/mem/array.h"
#include "lib/mem/buffer.h"
#include "lib/mem/slice.h"

static void readPosts(Blog* blog) {
    blog->mod_date = blog->posts_file->mod_date;
    Tokeniser lines = sliceTokeniserStr(mfileAsSlice(blog->posts_file), "\n", false);
    Slice line = nextToken(&lines);
    while (line.start && line.length > 0) {
        Tokeniser fields = sliceTokeniserStr(line, "\t", true);

        BlogPost* post = arrayAdd(blog->posts);
        post->dir = nextToken(&fields);
        post->title = nextToken(&fields);
        post->date = nextToken(&fields);

        Buffer* temp_path = bufNew(blog->allocator, 32);
        bufAppendStr(temp_path, "./blog/");
        bufAppendSlice(temp_path, post->dir);
        post->url = bufSlice(temp_path, 1, -1);

        bufAppendStr(temp_path, "/.post.html");
        post->content = allocateFile(blog->allocator, blog->polling, bufAsStr(temp_path));

        line = nextToken(&lines);
    }
}

Blog* blogNew(Allocator* allocator, Polling* polling) {
    Blog* blog = allocate(allocator, sizeof(Blog));
    blog->allocator = allocator;
    blog->polling = polling;
    blog->posts = arrayNew(allocator, sizeof(BlogPost), 32);

    blog->posts_file = allocateFile(allocator, polling, "./blog/.posts.dat");
    if (!blog->posts_file) {
        return NULL;
    }
    readPosts(blog);

    blog->header1 = allocateFile(allocator, polling, "./blog/.header1.html");
    blog->header2 = allocateFile(allocator, polling, "./blog/.header2.html");
    blog->footer = allocateFile(allocator, polling, "./blog/.footer.html");
    if (!blog->header1 || !blog->header2 || !blog->footer) {
        deallocateFile(allocator, blog->header1);
        deallocateFile(allocator, blog->header2);
        deallocateFile(allocator, blog->footer);
        return NULL;
    }

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
	bufAppendMFile(buf, post->content);
	bufAppendStr(buf, "</article>\n");
}

static time_t maxTime(time_t a, time_t b) {
	return a>=b ? a : b;
}
static time_t modDate(Blog* blog, bool posts) {
    time_t mod_date = blog->mod_date;

    mod_date = maxTime(mod_date, blog->header1->mod_date);
    mod_date = maxTime(mod_date, blog->header2->mod_date);
    mod_date = maxTime(mod_date, blog->footer->mod_date);

    if (posts) {
        for (U64 i=0; i<blog->posts->count; i++) {
            mod_date = maxTime(mod_date, ((BlogPost*)arrayGet(blog->posts, i))->content->mod_date);
        }
    }

    return mod_date;
}

bool blogContent(Blog* blog, HttpServerExchange* exchange) {
    // check for changes
    if (blog->mod_date < blog->posts_file->mod_date) {
        for (U64 i=0; i<blog->posts->count; i++) {
            deallocateFile(blog->allocator, ((BlogPost*)arrayGet(blog->posts, i))->content);
        }
        arrayReset(blog->posts);
        readPosts(blog);
    }

    // start content
    Buffer* content = bufNew(exchange->scope, KB(4));
    URL* target = exchange->request->target;

    // home page
    if (sliceIsStr(target->path, "/")) {
        if (!isGetOrHead(exchange)) {
            return true;
        }

        // check modified date
        const time_t mod_date = modDate(blog, true);
        if (exchange->request->if_modified_since > 0 && exchange->request->if_modified_since >= mod_date) {
            DEBUG("Use cached version of home page");
            httpServerSendNotModified(exchange);
            return true;
        }

        // generate content
		bufAppendMFile(content, blog->header1);
        bufAppendMFile(content, blog->header2);
        for (U64 i=0; i<blog->posts->count; i++) {
            if (i > 0) {
				bufAppendStr(content, "<hr>\n");
			}
            composeArticle(content, (BlogPost*)arrayGet(blog->posts, i));
        }
		bufAppendMFile(content, blog->footer);

        // send
        httpServerAddHeader(exchange, sliceFromStr("Cache-Control"), sliceFromStr("no-cache"));
		httpServerAddDateHeader(exchange, sliceFromStr("Last-Modified"), mod_date);
		httpServerSetContent(exchange, sliceFromStr("html"), bufAsSlice(content));
        httpServerSend(exchange);
        return true;
    }

    // log page
    if (target->path_segments->count == 1 && sliceIsStr(*(Slice*)arrayGet(target->path_segments, 0), "log")) {
        if (!isGetOrHead(exchange)) {
            return true;
        }

        // check modified date
        const time_t mod_date = modDate(blog, true);
        if (exchange->request->if_modified_since > 0 && exchange->request->if_modified_since >= mod_date) {
            httpServerSendNotModified(exchange);
            return true;
        }

        // generate content
		bufAppendMFile(content, blog->header1);
        bufAppendMFile(content, blog->header2);
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
		bufAppendMFile(content, blog->footer);

        // send
        httpServerAddHeader(exchange, sliceFromStr("Cache-Control"), sliceFromStr("no-cache"));
		httpServerAddDateHeader(exchange, sliceFromStr("Last-Modified"), mod_date);
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

            // check modified date
            const time_t mod_date = modDate(blog, false);
            if (exchange->request->if_modified_since > 0 && exchange->request->if_modified_since >= mod_date) {
                httpServerSendNotModified(exchange);
                return true;
            }

            // generate content
            bufAppendMFile(content, blog->header1);
            bufAppendStr(content, " - Blog");
            bufAppendMFile(content, blog->header2);
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
		    bufAppendMFile(content, blog->footer);

            // send
            httpServerAddHeader(exchange, sliceFromStr("Cache-Control"), sliceFromStr("no-cache"));
            httpServerAddDateHeader(exchange, sliceFromStr("Last-Modified"), mod_date);
            httpServerSetContent(exchange, sliceFromStr("html"), bufAsSlice(content));
            httpServerSend(exchange);
            return true;
        }

        // blog post
        if (target->path_segments->count == 2) {
            for (U64 i=0; i<blog->posts->count; i++) {
                BlogPost* post = (BlogPost*)arrayGet(blog->posts, i);
                if (sliceIs(*(Slice*)arrayGet(target->path_segments, 1), post->dir)) {
                    // check modified date
                    const time_t mod_date = maxTime(modDate(blog, false), post->content->mod_date);
                    if (exchange->request->if_modified_since > 0 && exchange->request->if_modified_since >= mod_date) {
                        httpServerSendNotModified(exchange);
                        return true;
                    }

                    // generate content
                    bufAppendMFile(content, blog->header1);
                    bufAppendFormat(content, " - %.*s", post->title.length, post->title.start);
                    bufAppendMFile(content, blog->header2);
                    bufAppendFormat(content, "<article><h1>%.*s</h1><h2>%.*s</h2>\n", post->title.length, post->title.start, post->date.length, post->date.start);
                    bufAppendMFile(content, post->content);
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
		            bufAppendMFile(content, blog->footer);

                    // send
                    httpServerAddHeader(exchange, sliceFromStr("Cache-Control"), sliceFromStr("no-cache"));
                    httpServerAddDateHeader(exchange, sliceFromStr("Last-Modified"), mod_date);
                    httpServerSetContent(exchange, sliceFromStr("html"), bufAsSlice(content));
                    httpServerSend(exchange);
                    return true;
                }
            }
        }
    }

    return false;
}