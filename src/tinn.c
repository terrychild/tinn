#include <stdlib.h>
#include <string.h>

#include "lib.h"
/*#include "utils.h"
#include "content_generator.h"
#include "blog.h"
#include "static.h"
#include "server.h"*/
#include "help.h"
#include "version.h"
#include "tests.h"
#include "web.h"

/*

struct settings_t {
    char* port;
    char* content_dir;
};

static struct settings_t parse_arguments(int count, char* values[]) {
    struct settings_t settings = {
        .port = "8080",
        .content_dir = "."
    };
    bool set_content_dir = false;

    for (int i=1; i<count; i++) {
        if (values[i][0] == '-') {
            int len = strlen(values[i]);
            if (len==1) {
                usage_exit();
            }
            if (values[i][1] == '-') {
                if (strcmp(values[i], "--help")==0) {
                    usage_exit();
                } else if (strcmp(values[i], "--version")==0) {
                    version_exit();
                } else if (strcmp(values[i], "--verbose")==0) {
                    clevel = CL_TRACE;
                }
            } else {
                if (values[i][1] == 'h') {
                    usage_exit();
                } else if (values[i][1] == 'v') {
                    clevel = CL_TRACE;
                } else if (values[i][1] == 'p') {
                    if (i==count-1) {
                        usage_exit();
                    }
                    settings.port = values[i+1];
                    i++;
                } else {
                    usage_exit();
                }
            }
        } else {
            if (set_content_dir) {
                usage_exit();
            }
            set_content_dir = true;
            settings.content_dir = values[i];
        }
    }

    return settings;
}*/

static void printVersion() {
    PRINT(CC_BRIGHT_WHITE, "Tinn %s ", VERSION);
    PRINT(CC_BRIGHT_BLACK, "(%s)\n", BUILD_DATE);
}

// ================ Main loop etc ================
int main(int argc, char* argv[]) {
    if (cliArg(argc, argv, "--verbose")) {
        clevel = CL_DEBUG;
    }

    if (argc<2) {
        printUsage();  
        return EXIT_FAILURE;
    }

    if (strcmp(argv[1], "help")==0) {
        return printHelp(argc, argv);
    }

    if (strcmp(argv[1], "version")==0) {
        printVersion();
        return EXIT_SUCCESS;
    }

    if (strcmp(argv[1], "test")==0) {
        return runTests();
    }

    if (strcmp(argv[1], "host")==0) {
        return hostWebServer(argc, argv);
    }

    PRINT(CC_BRIGHT_RED, "Error: ");
    PRINT(CC_WHITE, "Unknown command ");
    PRINT(CC_CYAN, "%s\n", argv[1]);
    return EXIT_FAILURE;

    /*
    // change working directory to content directory
    if (chdir(settings.content_dir) != 0) {
        ERROR("invalid content directory (%s)", settings.content_dir);
        return EXIT_FAILURE;
    }
    
    // create content generators
    TRACE("creating list of content generators");
    ContentGenerators* content = content_generators_new(2);

    Blog* blog = blog_new();
    if (blog != NULL) {
        content_generators_add(content, blog_content, blog);
    }
    
    content_generators_add(content, static_content, NULL);
    
    */
}
