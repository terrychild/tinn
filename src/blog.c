#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "utils.h"
#include "console.h"
#include "blog.h"
#include "buffer.h"

#include "posts_parser.h"

#define BLOG_DIR "blog"
#define MAX_PATH_LEN 256
#define MAX_DATE_LEN 20

struct post {
	char path[MAX_PATH_LEN];
	char title[MAX_PATH_LEN];
	char date[MAX_DATE_LEN];
	Buffer* content;
};

struct posts_list {
	int size;
	int count;
	struct post* data;
};

static struct posts_list* posts_list_new() {
	struct posts_list* list = allocate(NULL, sizeof(*list));
	list->size = 32;
	list->count = 0;
	list->data = allocate(NULL, sizeof(*list->data) * list->size);
	return list;
}
static void posts_list_free(struct posts_list* list) {
	if (list != NULL) {
		for (int i=0; i<list->count; i++) {
			buf_free(list->data[i].content);
		}
		free(list->data);
		free(list);
	}
}
static struct post* posts_list_add(struct posts_list* list) {
	if (list->count == list->size) {
		list->size *= 2;
		list->data = allocate(list->data, sizeof(*list->data) * list->size);
	}
	struct post* post = &(list->data[list->count++]);
	memset(post, 0, sizeof(*post));
	return post;
}

static bool read_posts(struct posts_list* posts) {
	TRACE("read blog posts");
	bool ok = true;

	char path[MAX_PATH_LEN];
	Buffer* buf = buf_new_file(BLOG_DIR "/.posts.dat");
	PostsParser parser = posts_parser_new(buf->data, buf->length);

	PostsToken dir;
	PostsToken title;
	PostsToken date;

	while (read_post(&parser, &dir, &title, &date)) {
		// validate
		int len = snprintf(path, MAX_PATH_LEN, "%s/%.*s/.post.html", BLOG_DIR, dir.length, dir.start);
		if (len < 0 || len >= MAX_PATH_LEN) {
			ERROR("Error, unable to create path for \"%.*s\"\n", dir.length, dir.start);
			ok = false;
			break;
		}
		if (title.length>MAX_PATH_LEN) {
			ERROR("Error, title too long \"%.*s\"\n", title.length, title.start);
			ok = false;
			break;
		}
		if (date.length>MAX_DATE_LEN) {
			ERROR("Error, date too long \"%.*s\"\n", date.length, date.start);
			ok = false;
			break;
		}

		// save
		struct post* post = posts_list_add(posts);

		snprintf(post->path, MAX_PATH_LEN, "/%s/%.*s", BLOG_DIR, dir.length, dir.start);
		memcpy(post->title, title.start, title.length);
		memcpy(post->date, date.start, date.length);

		post->content = buf_new_file(path);
		if (post->content == NULL) {
			ok = false;
			break;
		}
	}
	
	buf_free(buf);
	return ok;
}

static void compose_article(Buffer* buf, struct post post) {
	buf_append_str(buf, "<article>");
	buf_append_format(buf, "<h1><a href=\"%s\">%s</a></h1>", post.path, post.title);
	buf_append_format(buf, "<h2>%s</h2>", post.date);
	buf_append_buf(buf, post.content);
	buf_append_str(buf, "</article>\n");
}

bool blog_build(Routes* routes) {
	TRACE("build blog posts");
	bool ok = true;

	// load html fragments
	TRACE("loading html fragments");
	Buffer* header1 = buf_new_file(".header1.html");
	Buffer* header2 = buf_new_file(".header2.html");
	Buffer* footer = buf_new_file(".footer.html");

	ok = header1 != NULL && header2 != NULL && footer != NULL;

	// read blog list
	TRACE("read blog list");
	struct posts_list* posts = NULL;
	if (ok) {
		posts = posts_list_new();
		ok = read_posts(posts);
	}

	// build pages
	if (ok) {
		// build blog pages
		TRACE("build blog pages");
		for (int i=0; i<posts->count; i++) {
			TRACE("post %d \"%s\"", i, posts->data[i].title);
			Buffer* post = routes_new_buf(routes, posts->data[i].path, 10240);
			buf_append_buf(post, header1);
			buf_append_format(post, " - %s", posts->data[i].title);
			buf_append_buf(post, header2);
			buf_append_format(post, "<article><h1>%s</h1><h2>%s</h2>\n", posts->data[i].title, posts->data[i].date);
			buf_append_buf(post, posts->data[i].content);
			buf_append_str(post, "<nav>");
			if (i < posts->count-1) {
				buf_append_format(post, "<a href=\"%s\">prev</a>", posts->data[i+1].path);
			} else {
				buf_append_str(post, "<span>&nbsp;</span>");
			}
			if (i > 0) {
				buf_append_format(post, "<a href=\"%s\">next</a>", posts->data[i-1].path);
			}
			buf_append_str(post, "</nav></article>\n");
			buf_append_buf(post, footer);
		}

		// build archive page
		TRACE("build archive page");
		Buffer* archive = routes_new_buf(routes, "/" BLOG_DIR, 10240);
		buf_append_buf(archive, header1);
		buf_append_str(archive, " - Blog");
		buf_append_buf(archive, header2);
		buf_append_str(archive, "<article><h1>Blog Archive</h1>\n");
		buf_append_str(archive, "<p>If you, like me, sometimes want to read an entire blog in chronological order without any unnecessary navigating and/or scrolling back and forth, you can do that <a href=\"/log\">here</a>.</p>\n");

		char archive_date[15] = "";

		for (int i=0; i<posts->count; i++) {
			TRACE("post %d \"%s\"", i, posts->data[i].title);
			if (strcmp(archive_date, strchr(posts->data[i].date, ' ')+1) != 0) {
				strcpy(archive_date, strchr(posts->data[i].date, ' ')+1);

				buf_append_format(archive, "<hr>\n<h3>%s</h3>\n", archive_date);
			}
			buf_append_format(archive, "<p><a href=\"%s\">%s</a></p>\n", posts->data[i].path, posts->data[i].title);
		}

		buf_append_str(archive, "</article>");
		buf_append_buf(archive, footer);

		// build home page
		TRACE("build home page");
		Buffer* home = routes_new_buf(routes, "/", 10240);
		buf_append_buf(home, header1);
		buf_append_buf(home, header2);

		for (int i=0; i<posts->count; i++) {
			TRACE("post %d \"%s\"", i, posts->data[i].title);
			if (i > 0) {
				buf_append_str(home, "<hr>\n");
			}
			compose_article(home, posts->data[i]);
		}

		buf_append_buf(home, footer);

		// build log page
		TRACE("build log page");
		Buffer* log = routes_new_buf(routes, "/log", 10240);
		buf_append_buf(log, header1);
		buf_append_buf(log, header2);
		
		for (int i=posts->count-1; i>=0; i--) {
			TRACE("post %d \"%s\"", i, posts->data[i].title);
			if (i < posts->count-1) {
				buf_append_str(log, "<hr>\n");
			}
			compose_article(log, posts->data[i]);
		}

		buf_append_buf(log, footer);
	}
	
	// clean up
	TRACE("build blog clean up");
	posts_list_free(posts);
	buf_free(header1);
	buf_free(header2);
	buf_free(footer);

	// success?
	return ok;
}

#undef BLOG_DIR