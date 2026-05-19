#include <stdio.h>
#include <stdlib.h>

#include "gen/gen.h"
#include "server.h"

int main(int argc, char* argv[]) {
    (void) argc;
    (void) argv;

    const char* html = md_to_html(argv[1]);
    mdprev_host(12345, html);
    free((char*) html);
    return 0;
}
