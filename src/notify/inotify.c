#include <libgen.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/inotify.h>

#include "globals.h"

static int32_t ifd = -1;
static int32_t wfd = -1;

int32_t track(void) {
    ifd = inotify_init1(IN_NONBLOCK | IN_CLOEXEC);
    if (ifd == -1) {
        perror("inotify_init()");
        exit(1);
    }

    char* gpath = strdup(GENPATH);
    char* dir = dirname(gpath);
    wfd = inotify_add_watch(ifd, dir, IN_CLOSE_WRITE);
    if (wfd == -1) {
        perror("inotify_add_watch()");
    }

    free(gpath);
    return ifd;
}

void untrack(void) {
    inotify_rm_watch(ifd, wfd);
    close(ifd);
}
