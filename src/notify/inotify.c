#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/inotify.h>

#include "globals.h"

static int32_t ifd = -1;
static int32_t wfd = -1;

int32_t track(void) {
    ifd = inotify_init();
    if (ifd == -1) {
        perror("inotify_init()");
        exit(1);
    }

    wfd = inotify_add_watch(ifd, GENPATH, IN_CLOSE_WRITE);
    if (wfd == -1) {
        perror("inotify_add_watch()");
    }

    return ifd;
}

void untrack(void) {
    inotify_rm_watch(ifd, wfd);
    close(ifd);
}
