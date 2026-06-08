#include <stdio.h>
#include <stdlib.h>

#include "gen/gen.h"
#include "globals.h"
#include "server.h"

int main(int argc, char* argv[]) {
    (void) argc;
    (void) argv;

    GENPATH = argv[1];
    mdprev_host(12345, GENBODY);

    free((char*) GENBODY);
    return 0;
}
