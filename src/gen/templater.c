#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "cmark-gfm.h"

#include "autogen/mathjax.h"
#include "autogen/prism_css.h"
#include "autogen/prism_js.h"
#include "autogen/stylesheet.h"

const char
    HEADER[] = "<html><head><title>mdprev</title><style>" STYLESHEET_DATA
               "</style><style>" PRISM_CSS_DATA
               "</style><script>window.MathJax = {tex: {inlineMath: "
               "[['\\\\(','\\\\)']],displayMath: "
               "[['$$','$$'],['\\\\[','\\\\]']]},options: {skipHtmlTags: "
               "['script','noscript','style','textarea'],enableMenu: "
               "false},sre: {speech: 'none'}};</script><script>" MATHJAX_DATA
               "</script><script>" PRISM_JS_DATA "</script></head>";

const char FOOTER[] = "</body></html>";

static bool rewrite_math_blocks(cmark_node* node) {
    bool ret = false;
    if (cmark_node_get_type(node) != CMARK_NODE_CODE_BLOCK) {
        return false;
    }

    const char* info = cmark_node_get_fence_info(node);
    if (!info || strcmp(info, "math") != 0) {
        return false;
    }

    const char* src = cmark_node_get_literal(node);
    if (!src) {
        return false;
    }

    size_t len = strlen(src);
    char* wrapped = malloc(len + 7);
    if (!wrapped) {
        goto cleanup;
    }

    memcpy(wrapped, "\\[\n", 3);
    memcpy(wrapped + 3, src, len);
    memcpy(wrapped + 3 + len, "\\]\n", 3);
    wrapped[6 + len] = '\0';

    cmark_node* html = cmark_node_new(CMARK_NODE_HTML_BLOCK);
    cmark_node_set_literal(html, wrapped);
    cmark_node_replace(node, html);
    cmark_node_free(node);
    ret = true;

cleanup:
    free(wrapped);
    return ret;
}

static void custom_modifier(cmark_node* root) {
    cmark_iter* iter = cmark_iter_new(root);
    cmark_event_type ev = cmark_iter_next(iter);
    while (ev != CMARK_EVENT_DONE) {
        if (ev != CMARK_EVENT_ENTER) {
            goto next;
        }

        cmark_node* node = cmark_iter_get_node(iter);
        if (rewrite_math_blocks(node)) {
            goto next;
        }

    next:
        ev = cmark_iter_next(iter);
    }

    cmark_iter_free(iter);
}

void templater_wrap(cmark_node* root) {
    custom_modifier(root);

    cmark_node* header_node = cmark_node_new(NODE_HTML_BLOCK);
    cmark_node_set_literal(header_node, HEADER);

    cmark_node* footer_node = cmark_node_new(NODE_HTML_BLOCK);
    cmark_node_set_literal(footer_node, FOOTER);

    cmark_node_prepend_child(root, header_node);
    cmark_node_append_child(root, footer_node);
}
