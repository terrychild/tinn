#ifndef TINN_BLOG_H
#define TINN_BLOH_H

#include <stdbool.h>
#include "request.h"
#include "response.h"

bool blog_build();
bool blog_content(void* blog, Request* request, Response* Response);

#endif