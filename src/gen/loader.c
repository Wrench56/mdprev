#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include "gen/loader.h"

static bool read_all(int fd, u_int8_t* data, size_t len, size_t* out_len) {
    size_t total = 0;
    while (total < len) {
        ssize_t n = read(fd, data + total, len - total);
        if (n < 0) {
            if (errno == EINTR) {
                continue;
            }

            return false;
        }

        if (n == 0) {
            break;
        }

        total += (size_t) n;
    }

    *out_len = total;
    return true;
}

static bool load_heap(int fd, size_t len, file_buffer_t* out) {
    if (lseek(fd, 0, SEEK_SET) < 0) {
        return false;
    }

    unsigned char* data = malloc(len + 1);
    if (!data) {
        return false;
    }

    size_t actual_len = 0;
    if (!read_all(fd, data, len, &actual_len)) {
        free(data);
        return false;
    }

    data[actual_len] = '\0';

    out->data = data;
    out->len = actual_len;
    out->kind = FILE_BUFFER_HEAP;
    out->owned = data;
    out->owned_len = actual_len + 1;
    out->fd = -1;

    return true;
}

static bool load_mmap(int fd, size_t len, file_buffer_t* out) {
    if (len == 0) {
        return load_heap(fd, len, out);
    }

    void* data = mmap(NULL, len, PROT_READ, MAP_PRIVATE, fd, 0);
    if (data == MAP_FAILED) {
        return false;
    }

    out->data = data;
    out->len = len;
    out->kind = FILE_BUFFER_MMAP;
    out->owned = data;
    out->owned_len = len;
    out->fd = fd;

    return true;
}

bool loader_load(const char* path, size_t mmap_threshold, file_buffer_t* out) {
    if (!path || !out) {
        return false;
    }

    int fd = open(path, O_RDONLY | O_CLOEXEC);
    if (fd < 0) {
        return false;
    }

    struct stat st;
    if (fstat(fd, &st) != 0 || !S_ISREG(st.st_mode)) {
        close(fd);
        return false;
    }

    size_t len = (size_t) st.st_size;
    if (len > 0 && len >= mmap_threshold && load_mmap(fd, len, out)) {
        return true;
    }

    bool ok = load_heap(fd, len, out);
    close(fd);
    return ok;
}

void loader_free(file_buffer_t* file) {
    if (!file) {
        return;
    }

    if (file->kind == FILE_BUFFER_MMAP && file->owned) {
        munmap(file->owned, file->owned_len);

        if (file->fd >= 0) {
            close(file->fd);
        }
    } else if (file->kind == FILE_BUFFER_HEAP && file->owned) {
        free(file->owned);
    }
}
