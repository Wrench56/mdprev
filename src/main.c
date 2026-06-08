#include <stdio.h>
#include <stdlib.h>

#include "gen/gen.h"
#include "globals.h"
#include "server.h"

int main(int argc, char* argv[]) {
    (void) argc;
    (void) argv;

    if (argc != 2) {
        printf("mdprev\n\n");
        printf("Usage: mdprev <markdown file>\n");
        return 0;
    }
        
    GENPATH = argv[1];
    mdprev_host(12345, GENBODY);

    free((char*) GENBODY);
    return 0;
}
