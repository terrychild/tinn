#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "help.h"

void print_usage() {
	puts("\nUsage: tinn <command> [<args>]");
	puts("\nCommands");
	puts("  host      Host a tinn web server.");
	puts("  help      Display help.");
	puts("\nFor help on a specific command see \"tinn help <command>\".");
}

int print_help(int argc, char* argv[]) {
	if (argc==2) {
		print_usage();
		return EXIT_SUCCESS;
	}

	printf("No help available for command \"%s\".\n", argv[2]);
	return EXIT_FAILURE;
}