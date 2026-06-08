#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "cmark-gfm.h"

#include "autogen/mathjax.h"
#include "autogen/prism_css.h"
#include "autogen/prism_js.h"
#include "autogen/stylesheet.h"

typedef struct {
    const char* marker;
    const char* type_class;
    const char* title;
} alert_type_t;

static const alert_type_t ALERT_TYPES[] = {
    { "[!NOTE]", "note", "Note" },
    { "[!TIP]", "tip", "Tip" },
    { "[!IMPORTANT]", "important", "Important" },
    { "[!WARNING]", "warning", "Warning" },
    { "[!CAUTION]", "caution", "Caution" },
};

#define ALERT_TYPES_COUNT (sizeof(ALERT_TYPES) / sizeof(ALERT_TYPES[0]))

const char
    HEADER[] = "<html><head><title>mdprev</title><style>" STYLESHEET_DATA
               "</style><style>" PRISM_CSS_DATA
               "</style><script>"
               "window.MathJax={"
                 "tex:{"
                   "inlineMath:[['\\\\(','\\\\)']],"
                   "displayMath:[['$$','$$'],['\\\\[','\\\\]']]"
                 "},"
                 "options:{"
                   "skipHtmlTags:['script','noscript','style','textarea'],"
                   "enableMenu:false,"
                   "enableSpeech:false,"
                   "enableBraille:false,"
                   "enableExplorer:false,"
                   "enableAssistiveMml:false,"
                   "menuOptions:{settings:{"
                     "speech:false,"
                     "enrich:false,"
                     "explorer:false,"
                     "assistiveMml:false"
                   "}}"
                 "}"
               "};"
               "</script><script>" MATHJAX_DATA
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

static const alert_type_t* match_alert(const char* text) {
    for (size_t i = 0; i < ALERT_TYPES_COUNT; i++) {
        if (strcmp(text, ALERT_TYPES[i].marker) == 0) {
            return &ALERT_TYPES[i];
        }
    }

    return NULL;
}

static bool rewrite_alert_blocks(cmark_node* node) {
    if (cmark_node_get_type(node) != CMARK_NODE_BLOCK_QUOTE) {
        return false;
    }

    cmark_node* paragraph = cmark_node_first_child(node);
    if (paragraph == NULL ||
        cmark_node_get_type(paragraph) != CMARK_NODE_PARAGRAPH) {
        return false;
    }

    cmark_node* first_text = cmark_node_first_child(paragraph);
    if (first_text == NULL ||
        cmark_node_get_type(first_text) != CMARK_NODE_TEXT) {
        return false;
    }

    const char* marker_text = cmark_node_get_literal(first_text);
    if (!marker_text) {
        return false;
    }

    cmark_node* after_marker = cmark_node_next(first_text);
    if (!after_marker ||
        cmark_node_get_type(after_marker) != CMARK_NODE_SOFTBREAK) {
        return false;
    }

    const alert_type_t* alert = match_alert(marker_text);
    if (!alert) {
        return false;
    }

    cmark_node_unlink(first_text);
    cmark_node_free(first_text);
    cmark_node_unlink(after_marker);
    cmark_node_free(after_marker);

    char open_html[256];
    snprintf(
        open_html,
        sizeof(open_html),
        "<div class=\"markdown-alert markdown-alert-%s\">"
        "<p class=\"markdown-alert-title\">%s</p>",
        alert->type_class,
        alert->title
    );

    cmark_node* open_node = cmark_node_new(CMARK_NODE_HTML_BLOCK);
    cmark_node_set_literal(open_node, open_html);

    cmark_node* close_node = cmark_node_new(CMARK_NODE_HTML_BLOCK);
    cmark_node_set_literal(close_node, "</div>");

    cmark_node_insert_before(node, open_node);
    cmark_node_insert_after(node, close_node);

    cmark_node* child = cmark_node_first_child(node);
    while (child != NULL) {
        cmark_node_unlink(child);
        cmark_node_insert_before(node, child);
        child = cmark_node_first_child(node);
    }

    cmark_node_unlink(node);
    cmark_node_free(node);
    return true;
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
        if (rewrite_alert_blocks(node)) {
            goto next;
        }

    next:
        ev = cmark_iter_next(iter);
    }

    cmark_iter_free(iter);
}

void templater_wrap(cmark_node* root) {
    custom_modifier(root);

    cmark_node* header_node = cmark_node_new(CMARK_NODE_HTML_BLOCK);
    cmark_node_set_literal(header_node, HEADER);

    cmark_node* footer_node = cmark_node_new(CMARK_NODE_HTML_BLOCK);
    cmark_node_set_literal(footer_node, FOOTER);

    cmark_node_prepend_child(root, header_node);
    cmark_node_append_child(root, footer_node);
}
