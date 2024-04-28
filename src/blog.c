#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "utils.h"
#include "console.h"
#include "blog.h"
#include "buffer.h"

#include "scanner.h"

#define BLOG_DIR "blog"

static struct post* add_post(Blog* blog) {
	if (blog->count == blog->size) {
		blog->size *= 2;
		blog->posts = allocate(blog->posts, sizeof(*blog->posts) * blog->size);
	}
	struct post* post = &(blog->posts[blog->count++]);
	memset(post, 0, sizeof(*post));
	return post;
}

static void read_posts(Blog* blog) {
	TRACE("read blog posts");
	
	char path[BLOG_MAX_PATH_LEN];
	Buffer* buf = buf_new_file(BLOG_DIR "/.posts.dat");
	Scanner line_scanner = scanner_new(buf->data, buf->length);

	Token line;
	while ((line = scan_token(&line_scanner, "\r\n")).length>0) {
		Scanner field_scanner = scanner_new(line.start, line.length);

		Token dir = scan_token(&field_scanner, "\t");
		Token title = scan_token(&field_scanner, "\t");
		Token date = scan_token(&field_scanner, "\t");

		// validate
		if (dir.length==0 || title.length==0 || date.length==0) {
			ERROR("post line is invalid \"%.*s\"\n", line.length, line.start);
			continue;
		}
		int len = snprintf(path, BLOG_MAX_PATH_LEN, "%s/%.*s/.post.html", BLOG_DIR, dir.length, dir.start);
		if (len < 0 || len >= BLOG_MAX_PATH_LEN) {
			ERROR("unable to create path for \"%.*s\"\n", dir.length, dir.start);
			continue;
		}
		if (title.length>BLOG_MAX_PATH_LEN) {
			ERROR("title too long \"%.*s\"\n", title.length, title.start);
			continue;
		}
		if (date.length>BLOG_MAX_DATE_LEN) {
			ERROR("date too long \"%.*s\"\n", date.length, date.start);
			continue;
		}

		// read content
		Buffer* content = buf_new_file(path);
		if (content == NULL) {
			ERROR("unable to read content from \"%s\"", path);
			continue;
		}

		// save
		struct post* post = add_post(blog);

		memcpy(post->source, path, len);
		snprintf(post->path, BLOG_MAX_PATH_LEN, "/%s/%.*s", BLOG_DIR, dir.length, dir.start);
		memcpy(post->title, title.start, title.length);
		memcpy(post->date, date.start, date.length);

		post->content = content;
	}
	
	buf_free(buf);
}

Blog* blog_new() {
	Blog* blog = allocate(NULL, sizeof(*blog));
	blog->size = 32;
	blog->count = 0;
	blog->posts = allocate(NULL, sizeof(*blog->posts) * blog->size);

	// load html fragments
	TRACE("loading html fragments");
	blog->header1 = buf_new_file(".header1.html");
	blog->header2 = buf_new_file(".header2.html");
	blog->footer = buf_new_file(".footer.html");

	if (blog->header1 == NULL || blog->header2 == NULL || blog->footer == NULL) {
		buf_free(blog->header1);
		buf_free(blog->header2);
		buf_free(blog->footer);
		return NULL;
	}

	// read posts
	read_posts(blog);

	return blog;
}
void blog_free(Blog* blog) {
	if (blog != NULL) {
		for (size_t i=0; i<blog->count; i++) {
			buf_free(blog->posts[i].content);
		}
		free(blog->posts);
		buf_free(blog->header1);
		buf_free(blog->header2);
		buf_free(blog->footer);
		free(blog);
	}
}

static void compose_article(Buffer* buf, struct post post) {
	buf_append_str(buf, "<article>");
	buf_append_format(buf, "<h1><a href=\"%s\">%s</a></h1>", post.path, post.title);
	buf_append_format(buf, "<h2>%s</h2>", post.date);
	buf_append_buf(buf, post.content);
	buf_append_str(buf, "</article>\n");
}

bool blog_content(void* state, Request* request, Response* response) {
	TRACE("checking blog content");

	Blog* blog = (Blog*)state;

	if (strcmp(request->target->path, "/")==0) {
		TRACE("generate home page");

		response_status(response, 200);
		//response_header(response, "Cache-Control", "no-cache");
		//response_date(response, "Last-Modified", attrib.st_mtime);

		Buffer* content = response_content(response, "html");
		buf_append_buf(content, blog->header1);
		buf_append_buf(content, blog->header2);

		for (size_t i=0; i<blog->count; i++) {
			TRACE_DETAIL("post %d \"%s\"", i, blog->posts[i].title);
			if (i > 0) {
				buf_append_str(content, "<hr>\n");
			}
			compose_article(content, blog->posts[i]);
		}

		buf_append_buf(content, blog->footer);
		return true;
	}

	if (strcmp(request->target->path, "/log")==0) {
		TRACE("generate log page");

		response_status(response, 200);
		//response_header(response, "Cache-Control", "no-cache");
		//response_date(response, "Last-Modified", attrib.st_mtime);

		Buffer* content = response_content(response, "html");
		buf_append_buf(content, blog->header1);
		buf_append_buf(content, blog->header2);

		for (ssize_t i=blog->count-1; i>=0; i--) {
			TRACE_DETAIL("post %d \"%s\"", i, blog->posts[i].title);
			if (i < (ssize_t)blog->count-1) {
				buf_append_str(content, "<hr>\n");
			}
			compose_article(content, blog->posts[i]);
		}

		buf_append_buf(content, blog->footer);
		return true;
	}

	if (strcmp(request->target->path, "/" BLOG_DIR)==0) {
		TRACE("generate archive page");

		response_status(response, 200);
		//response_header(response, "Cache-Control", "no-cache");
		//response_date(response, "Last-Modified", attrib.st_mtime);

		Buffer* content = response_content(response, "html");
		buf_append_buf(content, blog->header1);
		buf_append_str(content, " - Blog");
		buf_append_buf(content, blog->header2);
		buf_append_str(content, "<article><h1>Blog Archive</h1>\n");
		buf_append_str(content, "<p>If you, like me, sometimes want to read an entire blog in chronological order without any unnecessary navigating and/or scrolling back and forth, you can do that <a href=\"/log\">here</a>.</p>\n");

		char archive_date[15] = "";

		for (size_t i=0; i<blog->count; i++) {
			TRACE_DETAIL("post %d \"%s\"", i, blog->posts[i].title);
			if (strcmp(archive_date, strchr(blog->posts[i].date, ' ')+1) != 0) {
				strcpy(archive_date, strchr(blog->posts[i].date, ' ')+1);

				buf_append_format(content, "<hr>\n<h3>%s</h3>\n", archive_date);
			}
			buf_append_format(content, "<p><a href=\"%s\">%s</a></p>\n", blog->posts[i].path, blog->posts[i].title);
		}

		buf_append_str(content, "</article>");
		buf_append_buf(content, blog->footer);
		return true;
	}

	for (size_t i=0; i<blog->count; i++) {
		if (strcmp(request->target->path, blog->posts[i].path)==0) {
			TRACE("generate \"%s\" page", blog->posts[i].title);
			
			response_status(response, 200);
			//response_header(response, "Cache-Control", "no-cache");
			//response_date(response, "Last-Modified", attrib.st_mtime);

			Buffer* content = response_content(response, "html");
			buf_append_buf(content, blog->header1);
			buf_append_format(content, " - %s", blog->posts[i].title);
			buf_append_buf(content, blog->header2);
			buf_append_format(content, "<article><h1>%s</h1><h2>%s</h2>\n", blog->posts[i].title, blog->posts[i].date);
			buf_append_buf(content, blog->posts[i].content);
			buf_append_str(content, "<nav>");
			if (i < blog->count-1) {
				buf_append_format(content, "<a href=\"%s\">prev</a>", blog->posts[i+1].path);
			} else {
				buf_append_str(content, "<span>&nbsp;</span>");
			}
			if (i > 0) {
				buf_append_format(content, "<a href=\"%s\">next</a>", blog->posts[i-1].path);
			}
			buf_append_str(content, "</nav></article>\n");
			buf_append_buf(content, blog->footer);
			return true;
		}
	}

	return false;
}

#undef BLOG_DIR