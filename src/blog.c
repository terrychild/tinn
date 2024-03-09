#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "console.h"
#include "blog.h"
#include "buffer.h"

#define BLOG_DIR "blog"
#define MAX_PATH_LEN 256

struct post {
	char path[MAX_PATH_LEN];
	char server_path[MAX_PATH_LEN];
	char title[MAX_PATH_LEN];
	char date[20];
	Buffer* content;
};

struct posts_list {
	size_t size;
	size_t count;
	struct post* data;
};

static struct posts_list* posts_list_new() {
	struct posts_list* list = malloc(sizeof(*list));
	if (list == NULL) {
		PANIC("unable to allocate memory for posts list");
	}
	list->size = 32;
	list->count = 0;
	list->data = malloc(sizeof(*list->data) * list->size);
	if (list->data == NULL) {
		PANIC("unable to allocate memory for posts list");
	}
	return list;
}
static void posts_list_free(struct posts_list* list) {
	if (list != NULL) {
		for (size_t i=0; i<list->count; i++) {
			buf_free(list->data[i].content);
		}
		free(list->data);
		free(list);
	}
}
static void posts_list_extend(struct posts_list* list, size_t new_size) {
	list->size = new_size;
	list->data = realloc(list->data, sizeof(*list->data) * new_size);
	if (list->data == NULL) {
		PANIC("unable to allocate memory for posts list");
	}
}
static struct post* posts_list_draft(struct posts_list* list) {
	if (list->count == list->size) {
		posts_list_extend(list, list->size * 2);
	}
	return &(list->data[list->count]);
}
static void posts_list_commit(struct posts_list* list) {
	if (list->count < list->size) {
		list->count++;
	}
}

static bool read_posts(struct posts_list* posts) {
	char path[MAX_PATH_LEN];
	snprintf(path, MAX_PATH_LEN, "%s/.posts.txt", BLOG_DIR);

	FILE* data = fopen(path, "r");
	if (data == NULL) {
		ERROR("unable to read %s\n", path);
		return false;
	}

	for (;;) {
		struct post* post = posts_list_draft(posts);
		if (fscanf(data, "%255[^\t]%*[\t]%2556[^\t]%*[\t]%19[^\n]%*[\n]", post->path, post->title, post->date) != 3) { //TODO better parsing and no hardcoding?
			// done reading file?
			break;
		} else {
			int len = snprintf(path, MAX_PATH_LEN, "%s/%s/.post.html", BLOG_DIR, post->path);
			if (len < 0 || len >= MAX_PATH_LEN) {
				ERROR("Error, unable to create path for \"%s\"\n", post->path);
				continue;
			}
			len = snprintf(post->server_path, MAX_PATH_LEN, "/%s/%s", BLOG_DIR, post->path);

			post->content = buf_new_file(path);
			if (post->content == NULL) {
				continue;
			}

			posts_list_commit(posts);
		}
	}

	fclose(data);
	return true;
}

static void compose_article(Buffer* buf, struct post post) {
	buf_append_str(buf, "<article>");
	buf_append_format(buf, "<h1><a href=\"%s\">%s</a></h1>", post.server_path, post.title);
	buf_append_format(buf, "<h2>%s</h2>", post.date);
	buf_append_buf(buf, post.content);
	buf_append_str(buf, "</article>\n");
}

bool blog_build(Routes* routes) {
	bool ok = true;

	// load html fragments
	Buffer* header1 = buf_new_file(".header1.html");
	Buffer* header2 = buf_new_file(".header2.html");
	Buffer* footer = buf_new_file(".footer.html");

	ok = header1 != NULL && header2 != NULL && footer != NULL;

	// read blog list
	struct posts_list* posts = NULL;
	if (ok) {
		posts = posts_list_new();
		ok = read_posts(posts);
	}

	// build pages
	if (ok) {
		// build blog pages
		for (size_t i=0; i<posts->count; i++) {
			Buffer* post = routes_new_buf(routes, posts->data[i].server_path, 10240);
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
		Buffer* archive = routes_new_buf(routes, "/" BLOG_DIR, 10240);
		buf_append_buf(archive, header1);
		buf_append_str(archive, " - Blog");
		buf_append_buf(archive, header2);
		buf_append_str(archive, "<article><h1>Blog Archive</h1>\n");
		buf_append_str(archive, "<p>If you, like me, sometimes want to read an entire blog in chronological order without any unnecessary navigating and/or scrolling back and forth, you can do that <a href=\"/log\">here</a>.</p>\n");

		char archive_date[15] = "";

		for (size_t i=0; i<posts->count; i++) {
			if (strcmp(archive_date, strchr(posts->data[i].date, ' ')+1) != 0) {
				strcpy(archive_date, strchr(posts->data[i].date, ' ')+1);

				buf_append_format(archive, "<hr>\n<h3>%s</h3>\n", archive_date);
			}
			buf_append_format(archive, "<p><a href=\"%s\">%s</a></p>\n", posts->data[i].server_path, posts->data[i].title);
		}

		buf_append_str(archive, "</article>");
		buf_append_buf(archive, footer);

		// build home page
		Buffer* home = routes_new_buf(routes, "/", 10240);
		buf_append_buf(home, header1);
		buf_append_buf(home, header2);

		for (size_t i=0; i<posts->count; i++) {
			if (i > 0) {
				buf_append_str(home, "<hr>\n");
			}
			compose_article(home, posts->data[i]);
		}

		buf_append_buf(home, footer);

		// build log page
		Buffer* log = routes_new_buf(routes, "/log", 10240);
		buf_append_buf(log, header1);
		buf_append_buf(log, header2);

		for (size_t i=posts->count; i>0; i--) {
			if (i < posts->count) {
				buf_append_str(log, "<hr>\n");
			}
			compose_article(log, posts->data[i-1]);
		}

		buf_append_buf(log, footer);
	}
	
	// clean up
	posts_list_free(posts);
	buf_free(header1);
	buf_free(header2);
	buf_free(footer);

	// success?
	return ok;
}

#undef BLOG_DIR