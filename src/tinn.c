#include <stdlib.h>

#include "lib.h"

int main(int argc, char* argv[]) {
    if (cliArg(argc, argv, "--verbose")) {
        clevel = CL_DEBUG;
    }

    PRINT(CC_BRIGHT_RED, "Error: ");
    PRINT(CC_WHITE, "Unknown command ");
    PRINT(CC_CYAN, "%s\n", argv[1]);
    return EXIT_FAILURE;
}