#ifndef GEN_LOADER_H
#define GEN_LOADER_H

#include <stdbool.h>
#include <stddef.h>

typedef enum file_buffer_kind {
    FILE_BUFFER_NONE = 0,
    FILE_BUFFER_HEAP,
    FILE_BUFFER_MMAP,
} file_buffer_kind_t;

typedef struct file_buffer {
    const unsigned char* data;
    size_t len;

    file_buffer_kind_t kind;

    void* owned;
    size_t owned_len;

    int fd;
} file_buffer_t;

bool loader_load(const char* path, size_t mmap_threshold, file_buffer_t* out);
void loader_free(file_buffer_t* file);

#endif /* GEN_LOADER_H */
