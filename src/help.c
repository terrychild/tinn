#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "help.h"
#include "console.h"

static void print_command(const char* args) {
    PRINT(CC_BLUE, "tinn ");
    PRINT(CC_CYAN, args);
}
static void print_arg(const char* arg, const char* descripton) {
    PRINT(CC_CYAN, "  %-17s ", arg);
    PRINT(CC_WHITE, "%s\n", descripton);
}
static void print_arg_cont(const char* descripton) {
    PRINT(CC_WHITE, "                    %s\n", descripton);
}

void print_usage() {
    PRINT(CC_WHITE, "Usage: ");
    print_command("<command> [<args>]\n\n");
    PRINT(CC_GREEN, "Commands\n");
    print_arg("host", "Host a tinn web server.");
    print_arg("help", "Display help.");
    print_arg("version", "Display the current version and build date.");
    print_arg("test", "Run test suite.");
    PRINT(CC_WHITE, "\nFor help on a specific command see: ");
    print_command("help <command>\n");
}

static void host_help() {
    print_command("tinn host [<args>]\n\n");
    PRINT(CC_WHITE, "Host a Tinn web server.\n\n");
    PRINT(CC_GREEN, "Arguments\n");
    print_arg("--dir=<path>", "Path for the content directoy, defaults to current");
    print_arg_cont("directory.");
    print_arg("--port=<num>", "Port to listen on, defaults to 8080.");
    print_arg("--verbose", "Enable verbose logging.");    
}

static void test_help() {
    print_command("test\n\n");
    puts("Runs a series of tests to ensure Tinn is (possibly) working.  It's hard to");
    puts("prove these things, but it will certainly flag any obvious errors.");
}

int print_help(int argc, char* argv[]) {
    if (argc<3) {
        print_usage();
        return EXIT_SUCCESS;
    }

    if (strcmp(argv[2], "host")==0) {
        host_help();
        return EXIT_SUCCESS;
    }

    if (strcmp(argv[2], "test")==0) {
        test_help();
        return EXIT_SUCCESS;
    }

    printf("No help available for command \"%s\".\n", argv[2]);
    return EXIT_FAILURE;
}