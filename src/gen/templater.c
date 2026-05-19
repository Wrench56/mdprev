#include "cmark-gfm.h"

#include "stylesheet.h"

const char HEADER[] = "<html><head><title>mdprev</title><style>" STYLESHEET_DATA
                      "</style></head>";

const char FOOTER[] = "</html>";

void templater_wrap(cmark_node* root) {
    cmark_node* header_node = cmark_node_new(NODE_HTML_BLOCK);
    cmark_node_set_literal(header_node, HEADER);

    cmark_node* footer_node = cmark_node_new(NODE_HTML_BLOCK);
    cmark_node_set_literal(footer_node, FOOTER);

    cmark_node_prepend_child(root, header_node);
    cmark_node_append_child(root, footer_node);
}
