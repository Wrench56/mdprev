#include <stdio.h>
#include <stdlib.h>

#include "gen/gen.h"
#include "globals.h"
#include "notify/inotify.h"
#include "server.h"

int main(int argc, char* argv[]) {
    (void) argc;
    (void) argv;

    GENPATH = argv[1];
    track();

    md_to_html();
    mdprev_host(12345, GENBODY);

    untrack();
    free((char*) GENBODY);
    return 0;
}
