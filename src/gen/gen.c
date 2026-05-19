#include <stdio.h>
#include <stdlib.h>

#include "cmark-gfm-core-extensions.h"
#include "cmark-gfm.h"

#include "gen/loader.h"
#include "gen/templater.h"

#define MMAP_THRESHOLD (16 * 1024 * 1024)
#define OPTS (CMARK_OPT_VALIDATE_UTF8 | CMARK_OPT_SMART | CMARK_OPT_UNSAFE)

#define LOAD_EXT(allocator, head, extension)                                   \
    cmark_syntax_extension* ext_##extension = cmark_find_syntax_extension(     \
        #extension                                                             \
    );                                                                         \
    if (ext_##extension == NULL) {                                             \
        fprintf(                                                               \
            stderr,                                                            \
            "Error: syntax extension \"%s\" does not exist!\n",                \
            #extension                                                         \
        );                                                                     \
        exit(1);                                                               \
    }                                                                          \
    (head) = cmark_llist_append(allocator, (head), ext_##extension);

static cmark_llist* get_extensions() {
    cmark_gfm_core_extensions_ensure_registered();

    cmark_mem* allocator = cmark_get_default_mem_allocator();

    cmark_llist* head = NULL;
    LOAD_EXT(allocator, head, tasklist);
    LOAD_EXT(allocator, head, table);
    LOAD_EXT(allocator, head, strikethrough);
    LOAD_EXT(allocator, head, autolink);

    return head;
}

static void free_extensions(cmark_llist* head) {
    cmark_mem* allocator = cmark_get_default_mem_allocator();

    cmark_llist_free_full(allocator, head, NULL);
}

char* md_to_html(const char* mdpath) {
    file_buffer_t file = { 0 };
    if (!loader_load(mdpath, MMAP_THRESHOLD, &file)) {
        fprintf(
            stderr,
            "Error: could not load markdown file with path \"%s\"\n",
            mdpath
        );
    }

    cmark_llist* ext_head = get_extensions();

    cmark_parser* parser = cmark_parser_new(OPTS);
    cmark_llist* node = ext_head;
    while (node != NULL) {
        cmark_parser_attach_syntax_extension(
            parser,
            (cmark_syntax_extension*) node->data
        );
        node = node->next;
    }

    cmark_parser_feed(parser, (const char*) file.data, file.len);
    cmark_node* root = cmark_parser_finish(parser);

    templater_wrap(root);
    char* html = cmark_render_html(root, OPTS, ext_head);

    free_extensions(ext_head);
    loader_free(&file);
    return html;
}
