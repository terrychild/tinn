#ifndef TINN_STATIC_H
#define TINN_STATIC_H

#include <stdbool.h>
#include "request.h"
#include "response.h"

bool static_content(void* state, Request* request, Response* Response);

#endif