#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>

#include "utils.h"
#include "console.h"
#include "blog.h"
#include "buffer.h"

#include "scanner.h"

#define BLOG_DIR "blog"
#define POSTS_PATH BLOG_DIR "/.posts.dat"

static time_t get_mod_date(const char* path) {
	struct stat attrib;
	if (stat(path, &attrib) == 0) {
		return attrib.st_mtime;
	}
	return 0;
}

static time_t max_time_t(time_t a, time_t b) {
	return a>=b ? a : b;
}

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

	// read file
	Buffer* buf = buf_new_file(POSTS_PATH);
	if (buf==NULL) {
		return;
	}

	// mod date
	blog->mod_date = get_mod_date(POSTS_PATH);

	// scan lines
	char path[BLOG_MAX_PATH_LEN];
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

		post->mod_date = get_mod_date(path);
		post->content = content;
	}
	
	buf_free(buf);
}

static void reread_posts(Blog* blog) {
	for (size_t i=0; i<blog->count; i++) {
		buf_free(blog->posts[i].content);
	}
	blog->count = 0;
	read_posts(blog);
}

static void check_post_date(struct post* post) {
	time_t mod_date = get_mod_date(post->source);
	if (mod_date > post->mod_date) {
		buf_reset(post->content);
		buf_append_file(post->content, post->source);
		post->mod_date = mod_date;
	}
}

static bool read_fragment(Blog* blog, size_t fragment, const char* path) {
	blog->fragments[fragment].path = path;
	blog->fragments[fragment].mod_date = get_mod_date(path);
	blog->fragments[fragment].buf = buf_new_file(path);

	return blog->fragments[fragment].buf != NULL;
}

Blog* blog_new() {
	Blog* blog = allocate(NULL, sizeof(*blog));
	blog->mod_date = 0;

	blog->size = 32;
	blog->count = 0;
	blog->posts = allocate(NULL, sizeof(*blog->posts) * blog->size);

	// load html fragments
	TRACE("loading html fragments");
	bool ok = true;
	ok = read_fragment(blog, HF_HEADER_1, ".header1.html") && ok;
	ok = read_fragment(blog, HF_HEADER_2, ".header2.html") && ok;
	ok = read_fragment(blog, HF_FOOTER, ".footer.html") && ok;
	if (!ok) {
		blog_free(blog);
		return NULL;
	}

	// read posts
	read_posts(blog);

	return blog;
}
void blog_free(Blog* blog) {
	if (blog != NULL) {
		for (size_t i=0; i<HF_COUNT; i++) {
			buf_free(blog->fragments[i].buf);
		}
		for (size_t i=0; i<blog->count; i++) {
			buf_free(blog->posts[i].content);
		}
		free(blog->posts);		
		free(blog);
	}
}

static void compose_article(Buffer* buf, struct post* post) {
	buf_append_str(buf, "<article>");
	buf_append_format(buf, "<h1><a href=\"%s\">%s</a></h1>", post->path, post->title);
	buf_append_format(buf, "<h2>%s</h2>", post->date);
	buf_append_buf(buf, post->content);
	buf_append_str(buf, "</article>\n");
}

bool blog_content(void* state, Request* request, Response* response) {
	TRACE("checking blog content");

	Blog* blog = (Blog*)state;

	// check for changes
	if (get_mod_date(POSTS_PATH) > blog->mod_date) {
		reread_posts(blog);
	}

	time_t mod_date = blog->mod_date;
	for (size_t i=0; i<HF_COUNT; i++) {
		time_t fragment_mod_date = get_mod_date(blog->fragments[i].path);
		if (fragment_mod_date > blog->fragments[i].mod_date) {
			buf_reset(blog->fragments[i].buf);
			buf_append_file(blog->fragments[i].buf, blog->fragments[i].path);
			blog->fragments[i].mod_date = fragment_mod_date;
		}
		mod_date = max_time_t(mod_date, fragment_mod_date);
	}

	// check home page
	if (strcmp(request->target->path, "/")==0) {
		TRACE("generate home page");

		// check modified date
		for (size_t i=0; i<blog->count; i++) {
			check_post_date(&(blog->posts[i]));
			mod_date = max_time_t(mod_date, blog->posts[i].mod_date);
		}

		if (request->if_modified_since>0 && request->if_modified_since>=mod_date) {
			TRACE("not modified, use cached version");
			response_status(response, 304);
			return true;
		}
		
		// generate page
		response_status(response, 200);
		response_header(response, "Cache-Control", "no-cache");
		response_date(response, "Last-Modified", mod_date);

		Buffer* content = response_content(response, "html");
		buf_append_buf(content, blog->fragments[HF_HEADER_1].buf);
		buf_append_buf(content, blog->fragments[HF_HEADER_2].buf);

		for (size_t i=0; i<blog->count; i++) {
			TRACE_DETAIL("post %d \"%s\"", i, blog->posts[i].title);
			if (i > 0) {
				buf_append_str(content, "<hr>\n");
			}
			compose_article(content, &(blog->posts[i]));
		}

		buf_append_buf(content, blog->fragments[HF_FOOTER].buf);
		return true;
	}

	// check log page
	if (strcmp(request->target->path, "/log")==0) {
		TRACE("generate log page");

		// check modified date
		for (size_t i=0; i<blog->count; i++) {
			check_post_date(&(blog->posts[i]));
			mod_date = max_time_t(mod_date, blog->posts[i].mod_date);
		}

		if (request->if_modified_since>0 && request->if_modified_since>=mod_date) {
			TRACE("not modified, use cached version");
			response_status(response, 304);
			return true;
		}
		
		// generate page
		response_status(response, 200);
		response_header(response, "Cache-Control", "no-cache");
		response_date(response, "Last-Modified", mod_date);

		Buffer* content = response_content(response, "html");
		buf_append_buf(content, blog->fragments[HF_HEADER_1].buf);
		buf_append_buf(content, blog->fragments[HF_HEADER_2].buf);

		for (ssize_t i=blog->count-1; i>=0; i--) {
			TRACE_DETAIL("post %d \"%s\"", i, blog->posts[i].title);
			if (i < (ssize_t)blog->count-1) {
				buf_append_str(content, "<hr>\n");
			}
			compose_article(content, &(blog->posts[i]));
		}

		buf_append_buf(content, blog->fragments[HF_FOOTER].buf);
		return true;
	}

	// check archive page
	if (strcmp(request->target->path, "/" BLOG_DIR)==0) {
		TRACE("generate archive page");

		// check modified date
		if (request->if_modified_since>0 && request->if_modified_since>=mod_date) {
			TRACE("not modified, use cached version");
			response_status(response, 304);
			return true;
		}
		
		// generate page
		response_status(response, 200);
		response_header(response, "Cache-Control", "no-cache");
		response_date(response, "Last-Modified", mod_date);

		Buffer* content = response_content(response, "html");
		buf_append_buf(content, blog->fragments[HF_HEADER_1].buf);
		buf_append_str(content, " - Blog");
		buf_append_buf(content, blog->fragments[HF_HEADER_2].buf);
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
		buf_append_buf(content, blog->fragments[HF_FOOTER].buf);
		return true;
	}

	// look for blog pages
	for (size_t i=0; i<blog->count; i++) {
		if (strcmp(request->target->path, blog->posts[i].path)==0) {
			TRACE("generate \"%s\" page", blog->posts[i].title);

			// check modified date
			check_post_date(&(blog->posts[i]));
			mod_date = max_time_t(mod_date, blog->posts[i].mod_date);

			if (request->if_modified_since>0 && request->if_modified_since>=mod_date) {
				TRACE("not modified, use cached version");
				response_status(response, 304);
				return true;
			}
			
			// generate page
			response_status(response, 200);
			response_header(response, "Cache-Control", "no-cache");
			response_date(response, "Last-Modified", mod_date);

			Buffer* content = response_content(response, "html");
			buf_append_buf(content, blog->fragments[HF_HEADER_1].buf);
			buf_append_format(content, " - %s", blog->posts[i].title);
			buf_append_buf(content, blog->fragments[HF_HEADER_2].buf);
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
			buf_append_buf(content, blog->fragments[HF_FOOTER].buf);
			return true;
		}
	}

	return false;
}

#undef BLOG_DIR
#undef POSTS_PATH