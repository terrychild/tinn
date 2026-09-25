#include "lib/types.h"
#include "lib/cli.h"

void hexDump(Slice slice) {
    bool skipped = false;
    for (U64 i=0; i < slice.length; i+=16) {
        bool has_data = false;
        for (U64 j=0; j<16 && !has_data; j++) {
            if (i+j < slice.length && slice.start[i+j] > 0) {
                has_data = true;
            }
        }

        if (!has_data) {
            skipped = true;
        } else {
            PRINT(skipped ? CC_BOLD_WHITE : CC_WHITE, "%08lX  ", i);
            skipped = false;
            for (U64 j=0; j<16; j++) {
                if (i+j < slice.length) {
                    PRINT(CC_YELLOW, "%02X ", slice.start[i+j]);
                } else {
                    PRINT(CC_BRIGHT_BLACK, ".. ");
                }
                if (j==7) {
                    PRINT(CC_NULL, " ");
                }
            }
            PRINT(CC_NULL, " ");

            for (U64 j=0; j<16; j++) {
                if (i+j < slice.length && slice.start[i+j] > 32 && slice.start[i+j] < 127) {
                    PRINT(CC_CYAN, "%c", slice.start[i+j]);
                } else {
                    PRINT(CC_NULL, " ");
                }
            }
            PRINT(CC_NULL, "\n");
        }
    }
}